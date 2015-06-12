crypto = require('crypto')
log = require('./log')
path = require('path')
Promise = require('bluebird')
rp = require('request-promise')
URL = require('url')
fs = Promise.promisifyAll(require('fs'), suffix:'Promised')
child_process = require('child-process-promise')


resource_dir = "/resource"
test_script_path =path.join(__dirname, "/judge_script.py")
data_root = '/usr/share/oj4th'

TASK_PAGE = '/judge/task'
FILE_PAGE = '/judge/file'
REPORT_PAGE = '/judge/report'


class judge_client
  self = undefined
  promiseWhile = (action) ->
    resolver = Promise.defer()
    my_loop = ->
      return resolver.resolve() if self.isStopped
      Promise.cast(action())
        .then(my_loop)
        .catch resolver.reject
    process.nextTick my_loop
    return resolver.promise

  constructor : (data)->
    @name = data.name
    @id = data.id
    @tmpfs_size = data.tmpfs_size || 500
    @cpu_mask = 0
    @cpu_mask += (1<<ele) for ele in data.cpu if data.cpu
    @task = undefined
    @secret_key = data.secret_key
    @create_time = data.create_time
    @log = new log(data.log_path)
    @config = JSON.stringify(data)
    @status = undefined
    @host = data.host
    @isStopped = false
    self = @
  send : (url, form = {})->
    post_time = new Date().toISOString()
    form.judge = {
      name: self.name
      post_time: new Date()
      token: crypto.createHash('sha1').update(self.secret_key + '$' + post_time).digest('hex')
    }
    rp
      .post( URL.resolve(self.host,url), {json:form})

  getTask : ->
    self.send(TASK_PAGE)

#util

  extract_file : (file_path)->
    child_process
      .spawn('python', ['./judge.py', 'resource', self.id, self.tmpfs_size, self.cpu_mask, file_path], {stdio:'inherit'})

#prepare
  pre_env : ->
    child_process
      .spawn('python', ['./judge.py', 'clean_all', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
      .then ->
        console.log "Pre_env finished"

  pre_submission : ->
    test_setting = ""
    for i of self.task.manifest.test_setting
      if self.task.manifest.test_setting[i] instanceof Array
        test_setting += "#{i} = #{self.task.manifest.test_setting[i].join(',')}\n"
      else
        test_setting += "#{i} = #{self.task.manifest.test_setting[i]}\n"
    inputFiles = (data.input for data in self.task.manifest.data)
    outputFiles = (data.output for data in self.task.manifest.data)
    weights = (data.weight for data in self.task.manifest.data)
    test_setting += "standard_input_files = #{inputFiles.join(',')}\n"
    test_setting += "standard_output_files = #{outputFiles.join(',')}\n"
    test_setting += "round_weight = #{weights.join(',')}\n"
    test_setting += "test_round_count = #{self.task.manifest.data.length}\n"
    child_process
      .spawn('python', ['./judge.py', 'prepare', self.id, self.tmpfs_size, self.cpu_mask, self.task.submission_code.content, self.task.lang, test_setting, test_script_path], {stdio:'inherit'})
      .then ->
        console.log "Pre_submission finished"

  get_file: (file_path)->
    console.log file_path
    self.send(FILE_PAGE,{
      problem_id : self.task.problem_id
      filename : self.task.manifest.test_setting.data_file
    })
    .pipe(fs.createWriteStream(file_path))

  pre_file: ->
    file_path = path.join(__dirname, resource_dir, self.task.manifest.test_setting.data_file)
    Promise.resolve()
      .then ->
        self.get_file file_path if not fs.existsSyncPromised file_path
      .then ->
        self.extract_file file_path
      .then ->
        console.log "Pre_file finished"

  prepare : ->
    Promise.resolve()
    .then ->
      self.pre_env()
    .then ->
      self.pre_submission()
    .then ->
      self.pre_file()
    .then ->
      return self.task

  judge : ->
    child_process
      .spawn('python', ['./judge.py', 'judge', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
      .then ->
        return self.task
  report : ->
    fs.readFilePromised(path.join(data_root,'work_'+self.id,'__report__'))
      .then (data)->
        detail = data.toString().split('\n')
        result_list = detail.shift().split(',')
        score = result_list[0]
        time_cost = result_list[1]
        memory_cost = result_list[2]
        result = detail.shift()
        detail = detail.join('\n')
        dictionary = {
          "Accepted" : "AC"
          "Wrong Answer" : "WA"
          "Compiler Error" : "CE"
          "Runtime Error (SIGSEGV)" : "RE"
          "Presentation Error" : "PE"
          "Memory Limit Exceed" : "MLE"
          "Time Limit Exceed" : "TLE"
        }
        console.log result
        report = {
          submission_id : self.task.id
          score : score
          time_cost : time_cost
          memory_cost : memory_cost
          result : dictionary[result]
          detail : detail
        }
        console.log report
        self.send(REPORT_PAGE, report)
  work : ->
    Promise.resolve()
      .then ->
        self.getTask()
      .then (task)->
        throw new Error("No task") if not task
        console.log task
        self.task = task
        self.prepare()
      .then ->
        self.judge()
      .then ->
        self.report()
      .catch (err)->
        console.log err.message
        Promise.delay(2000)

  init : ->
    process.on 'SIGTERM', ->
      self.stop()
    child_process
      .spawn('python', ['./judge.py', 'mount', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
      .then ->
        console.log "Mount finished"
      .then ->
        self.start()
      .catch (err)->
        console.log err
  stop : ->
    self.isStopped = true



  start : ->
    self.isStopped = false
    promiseWhile(self.work)
    .then ->
      child_process
        .spawn('python', ['./judge.py', 'umount', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
    .then ->
      process.disconnect && process.disconnect()

    .catch (err)->
      console.log err
module.exports = judge_client
