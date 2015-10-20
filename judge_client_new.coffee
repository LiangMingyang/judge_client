crypto = require('crypto')
log = require('./log')
path = require('path')
Promise = require('bluebird')
rp = require('request-promise')
URL = require('url')
promisePipe = require("promisepipe")
fs = Promise.promisifyAll(require('fs'), suffix:'Promised')
child_process = require('child-process-promise')
child_process_promised = Promise.promisifyAll(require('child_process'), suffix:'Promised')


resource_dirname = "resource"
data_dirname = "data"
work_dirname = "work"
submission_dirname = "submission"
utils_dirname = "utils"

TASK_PAGE = '/judge/task'
FILE_PAGE = '/judge/file'
REPORT_PAGE = '/judge/report'

class PipeError extends Error
  constructor: (@message = "PipeError.") ->
    @name = 'PipeError'
    Error.captureStackTrace(this, PipeError)

class NoTask extends Error
  constructor: (@message = "No task to judge.") ->
    @name = 'NoTask'
    Error.captureStackTrace(this, NoTask)


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
    @cpu_set = data.cpu_set
    @memory_limit = data.memory_limit
    @task = undefined
    @secret_key = data.secret_key
    @create_time = data.create_time
    @log = new log(data.log_path)
    @config = JSON.stringify(data)
    @status = undefined
    @host = data.host
    @isStopped = false
    @website = data.website
    self = @
  send : (url, form = {})->
    post_time = new Date().toISOString()
    form.judge = {
      id : self.id
      name: self.name
      post_time: post_time
      token: crypto.createHash('sha1').update(self.secret_key + '$' + post_time).digest('hex')
    }
    rp
      .post( URL.resolve(self.host,url), {json:form})
  getTask : ->
    self.send(TASK_PAGE)

##util
#
#  extract_file : (file_path)->
#    child_process
#      .spawn('python', ['./judge.py', 'resource', self.id, self.tmpfs_size, self.cpu_mask, file_path], {stdio:'inherit'})

##prepare
#  pre_env : ->
#    child_process
#      .spawn('python', ['./judge.py', 'clean_all', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
#      .then ->
#        console.log "Pre_env finished"

  pre_submission : ->
    test_setting = ""
#    for i of self.task.manifest.test_setting
#      if self.task.manifest.test_setting[i] instanceof Array
#        test_setting += "#{i} = #{self.task.manifest.test_setting[i].join(',')}\n"
#      else
#        test_setting += "#{i} = #{self.task.manifest.test_setting[i]}\n"
    if self.task.test_setting.data.length > 0
      inputFiles = (data.input for data in self.task.test_setting.data)
      outputFiles = (data.output for data in self.task.test_setting.data)
      weights = (data.weight for data in self.task.test_setting.data)
      test_setting += "standard_input_files = #{inputFiles.join(',')}\n"
      test_setting += "standard_output_files = #{outputFiles.join(',')}\n"
      test_setting += "round_weight = #{weights.join(',')}\n"
    test_setting += "support_lang = #{self.task.test_setting.supported_languages}\n"
    test_setting += "test_round_count = #{Math.max(1,self.task.test_setting.data.length)}\n"
    test_setting += "time_limit_case = #{self.task.test_setting.time_limit}\n"
    test_setting += "time_limit_global = #{self.task.test_setting.time_limit*self.task.test_setting.data.length}\n"
    test_setting += "memory_limit = #{self.task.test_setting.memory_limit}\n"
    work_path = path.resolve(__dirname, work_dirname, self.name)
    Promise.all [
      fs.writeFilePromised(path.resolve(work_path, submission_dirname,'__main__'), self.task.submission_code.content)
    ,
      fs.writeFilePromised(path.resolve(work_path, submission_dirname,'__lang__'), self.task.lang)
    ,
      fs.writeFilePromised(path.resolve(work_path, data_dirname,'__setting_code__'), test_setting)
    ]
    .then ->
      console.log "Pre_submission finished"

  get_file: (file_path)->
    if fs.existsSync file_path
      return
    form = {
      problem_id : self.task.problem_id
      filename : self.task.test_setting.data_file
    }
    post_time = new Date().toISOString()
    form.judge = {
      id : self.id
      name: self.name
      post_time: post_time
      token: crypto.createHash('sha1').update(self.secret_key + '$' + post_time).digest('hex')
    }
    #rp.post( URL.resolve(self.host, FILE_PAGE), {json:form}).pipe(ws)
    promisePipe(rp.post( URL.resolve(self.host, FILE_PAGE), {json:form}), fs.createWriteStream(file_path))
    .then( ->
      console.log "Piped successfully"
    ,
      (err)->
        if err
          fs.unlinkSync(file_path) if fs.existsSync file_path
          throw new PipeError()
    )

  pre_file: ->
    self.file_path = path.join(__dirname, resource_dirname, "#{self.website}", "#{self.task.test_setting.data_file}")
    Promise.resolve()
      .then ->
        self.get_file self.file_path
      .then ->
        console.log "Pre_file finished"

  prepare : ->
    Promise.resolve()
    .then ->
      self.pre_submission()
    .then ->
      self.pre_file()
    .then ->
      return self.task

  judge : ->
    utils_path = path.resolve(__dirname, utils_dirname)
    work_path = path.resolve(__dirname, work_dirname, self.name)
    file_path = self.file_path
    child_process
      .spawn('python', ['./judge.py', self.id, self.memory_limit, self.cpu_set.join(','), utils_path, work_path, file_path], {stdio:'inherit'})
      .then ->
        return self.task
  report : ->
    work_path = path.resolve(__dirname, work_dirname, self.name)
    fs.readFilePromised(path.join(work_path,'__report__'))
      .then (data)->
        detail = data.toString().split('\n')
        result_list = detail.shift().split(',')
        score = result_list[0]
        time_cost = result_list[1]
        memory_cost = result_list[2]
        result = detail.shift()
        detail = detail.join('\n')
        detail = "" if detail is '\n'
        console.log result
        dictionary = {
          "Accepted" : "AC"
          "Wrong Answer" : "WA"
          "Compiler Error" : "CE"
          "Runtime Error (SIGSEGV)" : "REG"
          "Runtime Error (SIGKILL)" : "MLE"
          "Runtime Error (SIGFPE)" : "REP"
          "Presentation Error" : "PE"
          "Memory Limit Exceed" : "MLE"
          "Time Limit Exceed" : "TLE"
          "Input File Not Ready" : "IFNR"
          "Output File Not Ready" : "OFNR"
          "Error File Not Ready" : "EFNR"
          "Other Error" : "OE"
        }
        if dictionary[result] is undefined
          detail = "#{result}\n#{detail}"
          result = "OE"
        else
          result = dictionary[result]
        report = {
          submission_id : self.task.id
          score : score
          time_cost : time_cost
          memory_cost : memory_cost
          result :  result
          detail : detail
        }
        self.send(REPORT_PAGE, report)
  work : ->
    Promise.resolve()
      .then ->
        self.getTask()
      .then (task)->
        throw new NoTask() if not task
        self.task = task
        self.prepare()
      .then ->
        self.judge()
      .then (report_data)->
        self.report(report_data)
      .catch NoTask, ->
        #console.log err.message
        Promise.delay(2000)
      .catch PipeError, (err)->
        console.log err
        report = {
          submission_id : self.task.id
          score : 0
          time_cost : 0
          memory_cost : 0
          result :  "WT"
          detail : "Rejudging"
        }
        self.send(REPORT_PAGE, report)
      .catch (err)->
        console.log err.message
        Promise.delay(1000)

  mkdir : ->
    work_path = path.resolve(__dirname, work_dirname, self.name)
    data_path = path.resolve(work_path, data_dirname)
    submission_path = path.resolve(work_path, submission_dirname)
    resource_path = path.resolve(__dirname, resource_dirname, self.website)
    child_process_promised.execPromised("mkdir -p #{data_path} #{submission_path} #{resource_path}")

  init : ->
    process.on 'SIGTERM', ->
      self.stop()
    Promise.resolve()
    .then ->
      self.mkdir()
    .then ->
      self.start()
    .then ->
      process.disconnect && process.disconnect()
      console.log "Stopped."
    .catch (err)->
      console.log err

  stop : ->
    self.isStopped = true

  start : ->
    self.isStopped = false
    promiseWhile(self.work)

module.exports = judge_client
