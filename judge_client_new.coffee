io = require('socket.io-client')
crypto = require('crypto')
async = require('async')
log = require('./log')
child_process = require('child_process')
ss = require('socket.io-stream')
path = require('path')
fs = require('fs')
http = require('http')
querystring = require('querystring')
resource_dir = "/resource"
test_script_path =path.join(__dirname, "/judge_script.py")
data_root = '/usr/share/oj4th'



class judge_client
  self = undefined
  constructor : (data)->
    @name = data.name
    @id = data.id
    @tmpfs_size = data.tmpfs_size || 500
    @cpu_mask = 0
    @cpu_mask += (1<<ele) for ele in data.cup if data.cpu
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
    postData = querystring.stringify form

    options = {
      hostname: self.host
      port: 3000
      path: url
      method: 'POST',
      headers:
        'Content-Type': 'application/x-www-form-urlencoded',
        'Content-Length': postData.length
    }


    req = http.request options, (res)->
      res.setEncoding('utf8')
      self.task = ""
      res.on 'data', (chunk)->
        self.task += chunk
      res.on 'end', ()->
        self.task = JSON.parse(self.task)
        console.log self.task.test_setting

    req.on 'error', (e)->
      console.log('problem with request: ' + e.message)

    req.write(postData)
    req.end()



myJudge = new judge_client({
  host : '127.0.0.1'
})
myJudge.send('/judge/task')