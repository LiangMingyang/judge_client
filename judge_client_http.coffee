io = require('socket.io-client');
crypto = require('crypto');
async = require('async');
log = require('./log');
child_process = require('child_process');
ss = require('socket.io-stream');
path = require('path');
fs = require('fs');
http = require('http')

resource_dir = "/resource";
test_script_path =path.join(__dirname, "/judge_script.py");
data_root = '/usr/share/oj4th';



class judge_client_http
  self = undefined
  constructor : (data)->
    @name = data.name;
    @id = data.id;
    @tmpfs_size = data.tmpfs_size || 500;
    @cpu_mask = 0;
    for ele in data.cup
      @cpu_mask += (1<<ele)
    @task = undefined;
    @secret_key = data.secret_key;
    @create_time = data.create_time;
    @log = new log(data.log_path);
    @config = JSON.stringify(data);
    @status = undefined;
    @url = data.host;
    self = @
  send : (url, form)->
    post_time = new Date().toISOString()
    form.judge =
      name: self.name
      post_time: new Date()
      token: crypto.createHash('sha1').update(self.secret_key + '$' + post_time).digest('hex')
    postData = querystring.stringify form

    options =
      hostname: self.host
      port: 80
      path: url
      method: 'POST',
      headers:
        'Content-Type': 'application/x-www-form-urlencoded',
        'Content-Length': postData.length


    req = http.request options, (res)->
      res.setEncoding('utf8')
      res.on 'data', (chunk)->
        console.log('BODY: ' + chunk);

    req.on 'error', (e)->
      console.log('problem with request: ' + e.message)

    req.write(postData);
    req.end();



