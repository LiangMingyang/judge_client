crypto = require('crypto')
log = require('./log')
path = require('path')
Promise = require('bluebird')
rp = require('request-promise')
URL = require('url')
fs = sequelize.Promise.promisifyAll(require('fs'), suffix:'Promised')
child_process = require('child-process-promise')


resource_dir = "/resource"
test_script_path =path.join(__dirname, "/judge_script.py")
data_root = '/usr/share/oj4th'

TASK_PAGE = '/judge/task'
FILE_PAGE = '/judge/file'


class judge_client
  self = undefined
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

  getTask : ()->
    self.send(TASK_PAGE)

#util

  extract_file : (file_path)->
    child_process
      .spawn('python', ['./judge.py', 'resource', self.id, self.tmpfs_size, self.cpu_mask, file_path], {stdio:'inherit'})

#prepare
  pre_env : ()->
    child_process
      .spawn('python', ['./judge.py', 'clean_all', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
      .then ()->
        console.log "Pre_env finished"

  pre_submission : ()->
    test_setting = ""
    for i of self.task.test_setting
      if self.task.test_setting[i] instanceof Array
        test_setting += "#{i} = #{self.task.test_setting[i].join(',')}\n"
      else
        test_setting += "#{i} = #{self.task.test_setting[i]}\n"
    self.task.test_setting = test_setting;

    child_process
      .spawn('python', ['./judge.py', 'prepare', self.id, self.tmpfs_size, self.cpu_mask, self.task.source_code, self.task.source_lang, self.task.test_setting, test_script_path], {stdio:'inherit'})
      .then ->
        console.log "Pre_submission finished"

  get_file: (file_path)->
    self.send(FILE_PAGE,{
      problem_id : self.task.problem_id
      filename : self.task.test_setting.data_file
    })
    .pipe(fs.createWriteStream(file_path))

  pre_file: ()->
    file_path = path.join(__dirname, resource_dir, self.task.filename)
    fs.existsPromised file_path
      .then (exits)->
        self.get_file if not exits
      .then ->
        self.extract_file file_path
      .then ->
        console.log "Pre_file finished"

  prepare : ()->
    Promise.resolve()
    .then ->
      self.pre_env()
    .then ->
      self.pre_submission()
    .then ->
      self.pre_file()

  judge : ()->

  report : ()->

  work : ()->
    Promise.resolve()
      .then ->
        self.getTask()
      .then (task)->
        self.task = task if task
        self.prepare() if task
      .then (task)->
        self.judge() if task
      .then (task)->
        self.report() if task

      .catch (err)->
        console.log err

  init : ()->
    child_process
      .spawn('python', ['./judge.py', 'mount', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
      .then ()->
        console.log "Mount finished"
      .catch (err)->
        console.log err

myJudge = new judge_client({
  host : 'http://127.0.0.1:3000'
  name : "judge0"
  id : 1
  tmpfs_size : 200
  cpu : [0,1]
})
myJudge.init()
#myJudge.send('/judge/task')