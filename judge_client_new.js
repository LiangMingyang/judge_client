// Generated by CoffeeScript 1.9.3
(function() {
  var FILE_PAGE, Promise, TASK_PAGE, URL, child_process, crypto, data_root, fs, judge_client, log, myJudge, path, resource_dir, rp, test_script_path;

  crypto = require('crypto');

  log = require('./log');

  path = require('path');

  Promise = require('bluebird');

  rp = require('request-promise');

  URL = require('url');

  fs = sequelize.Promise.promisifyAll(require('fs'), {
    suffix: 'Promised'
  });

  child_process = require('child-process-promise');

  resource_dir = "/resource";

  test_script_path = path.join(__dirname, "/judge_script.py");

  data_root = '/usr/share/oj4th';

  TASK_PAGE = '/judge/task';

  FILE_PAGE = '/judge/file';

  judge_client = (function() {
    var self;

    self = void 0;

    function judge_client(data) {
      var ele, j, len, ref;
      this.name = data.name;
      this.id = data.id;
      this.tmpfs_size = data.tmpfs_size || 500;
      this.cpu_mask = 0;
      if (data.cpu) {
        ref = data.cpu;
        for (j = 0, len = ref.length; j < len; j++) {
          ele = ref[j];
          this.cpu_mask += 1 << ele;
        }
      }
      this.task = void 0;
      this.secret_key = data.secret_key;
      this.create_time = data.create_time;
      this.log = new log(data.log_path);
      this.config = JSON.stringify(data);
      this.status = void 0;
      this.host = data.host;
      self = this;
    }

    judge_client.prototype.send = function(url, form) {
      var post_time;
      if (form == null) {
        form = {};
      }
      post_time = new Date().toISOString();
      form.judge = {
        name: self.name,
        post_time: new Date(),
        token: crypto.createHash('sha1').update(self.secret_key + '$' + post_time).digest('hex')
      };
      return rp.post(URL.resolve(self.host, url), {
        json: form
      });
    };

    judge_client.prototype.getTask = function() {
      return self.send(TASK_PAGE);
    };

    judge_client.prototype.extract_file = function(file_path) {
      return child_process.spawn('python', ['./judge.py', 'resource', self.id, self.tmpfs_size, self.cpu_mask, file_path], {
        stdio: 'inherit'
      });
    };

    judge_client.prototype.pre_env = function() {
      return child_process.spawn('python', ['./judge.py', 'clean_all', self.id, self.tmpfs_size, self.cpu_mask], {
        stdio: 'inherit'
      }).then(function() {
        return console.log("Pre_env finished");
      });
    };

    judge_client.prototype.pre_submission = function() {
      var i, test_setting;
      test_setting = "";
      for (i in self.task.test_setting) {
        if (self.task.test_setting[i] instanceof Array) {
          test_setting += i + " = " + (self.task.test_setting[i].join(',')) + "\n";
        } else {
          test_setting += i + " = " + self.task.test_setting[i] + "\n";
        }
      }
      self.task.test_setting = test_setting;
      return child_process.spawn('python', ['./judge.py', 'prepare', self.id, self.tmpfs_size, self.cpu_mask, self.task.source_code, self.task.source_lang, self.task.test_setting, test_script_path], {
        stdio: 'inherit'
      }).then(function() {
        return console.log("Pre_submission finished");
      });
    };

    judge_client.prototype.get_file = function(file_path) {
      return self.send(FILE_PAGE, {
        problem_id: self.task.problem_id,
        filename: self.task.test_setting.data_file
      }).pipe(fs.createWriteStream(file_path));
    };

    judge_client.prototype.pre_file = function() {
      var file_path;
      file_path = path.join(__dirname, resource_dir, self.task.filename);
      return fs.existsPromised(file_path).then(function(exits) {
        if (!exits) {
          return self.get_file;
        }
      }).then(function() {
        return self.extract_file(file_path);
      }).then(function() {
        return console.log("Pre_file finished");
      });
    };

    judge_client.prototype.prepare = function() {
      return Promise.resolve().then(function() {
        return self.pre_env();
      }).then(function() {
        return self.pre_submission();
      }).then(function() {
        return self.pre_file();
      });
    };

    judge_client.prototype.judge = function() {};

    judge_client.prototype.report = function() {};

    judge_client.prototype.work = function() {
      return Promise.resolve().then(function() {
        return self.getTask();
      }).then(function(task) {
        if (task) {
          self.task = task;
        }
        if (task) {
          return self.prepare();
        }
      }).then(function(task) {
        if (task) {
          return self.judge();
        }
      }).then(function(task) {
        if (task) {
          return self.report();
        }
      })["catch"](function(err) {
        return console.log(err);
      });
    };

    judge_client.prototype.init = function() {
      return child_process.spawn('python', ['./judge.py', 'mount', self.id, self.tmpfs_size, self.cpu_mask], {
        stdio: 'inherit'
      }).then(function() {
        return console.log("Mount finished");
      })["catch"](function(err) {
        return console.log(err);
      });
    };

    return judge_client;

  })();

  myJudge = new judge_client({
    host: 'http://127.0.0.1:3000',
    name: "judge0",
    id: 1,
    tmpfs_size: 200,
    cpu: [0, 1]
  });

  myJudge.init();

}).call(this);

//# sourceMappingURL=judge_client_new.js.map
