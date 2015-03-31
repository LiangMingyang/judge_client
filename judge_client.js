var io = require('socket.io-client');
var crypto = require('crypto');
var async = require('async');
var log = require('./log');
var child_process = require('child_process');
var ss = require('socket.io-stream');

var res_dir = "./res";


function judge_client(data, callback) {
    this.name = data.name;
    this.id = data.id;
    this.tmpfs_size = data.tmpfs_size || 500;
    this.cpu_mask = 0;
    data.cpu.forEach(function (ele) {
        this.cpu_mask += (1 << ele);
    });
    this.secret_key = data.secret_key;
    this.create_time = data.create_time;
    this.log = new log(data.log_path);
    this.config = JSON.stringify(data);
    this.status = undefined;
    this.url = data.url;
    this.socket = undefined;

    var self = this;

    this.change_status = function (status, message, callback) {
        self.status = status;
        self.socket.emit('status', self.status, function () {
            self.log.write(message);
            callback && callback();
        });
    };

    /**
     * start the process
     */
    this.start = function (callback) {
        child_process
            .spawn('python', ['./judge.py', 'mount', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
            .on('exit', function () {
                self.change_status('ready', 'mounted');
            });
        self.socket = io.connect(self.url)
            .on('connect', function () {
                self.log.write('connected with ' + self.config);
                self.log_in();
            })
            .on('message', function (msg) {
                self.log.write('Receive a piece of message: ' + msg);
            })
            .on('disconnect', function () {
                self.log.write('Disconnected');
            })
            .on('connect_error', function (err) {
                self.log.write('Error! err.message = ' + err.message);
            })
            .on('connect_timeout', function () {
                self.log.write('Connection timeout');
            })
            .on('reconnect', function (num) {
                self.log.write('Reconnect with number:' + num);
            })
            .on('reconnect_attempt', function () {
                self.log.write('Trying reconnect');
            })
            .on('task', function (task, confirm) {
                self.log.write('Got a task:' + task);
                self.work(task);
                confirm();
            })
            .on('command', function (command, confirm) {
                self.log.write('Receive command');
                confirm();
                //self.exec(command)
            });
        process.on('SIGTERM', function () {
            if (self.socket === undefined) return;
            self.stop(function () {
                process.disconnect && process.disconnect();
            });
        });

    };

    /**
     * stop the process
     * @param callback
     */

    this.stop = function (callback) {
        if(self.status == 'ready') {

        } else {
            self.fail(callback);
        }
        child_process
            .spawn('python', ['./judge.py', 'unmount', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
            .on('exit', function () {
                callback && callback();
            });
    };

    /**
     * when failed report failed
     * @param callback
     */

    this.fail = function (callback) {
        // TODO: fail the work
        console.log('failed');
        callback && callback();
    };

    /**
     * log in on connect event
     */
    this.log_in = function () {
        var post_time = new Date().toISOString();
        self.socket.emit('login', {
            name: self.name,
            post_time: post_time,
            token: crypto.createHash('sha1').update(self.secret_key + '$' + post_time).digest('hex'),
            status: self.status
        }, function (err) {
            if (err) {
                self.log.write(err.message);
                return;
            }
            self.log.write('Logged in');
        });
    };

    /**
     * prepare environment for judging
     * @param callback
     */

    this.pre_env = function (callback) {
        child_process
            .spawn('python', ['./judge.py', 'clean_all', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
            .on('exit', function () {
                callback && callback();
            });
    };
    /**
     * extract_file extract file from file_path to judge_res
     * @param file_path
     * @param callback
     */
    this.extract_file = function (file_path, callback) {
        // TODO: extract to the file_dir
    };
    /**
     * prepare files for judging
     * @param task
     * @param callback
     */
    this.pre_file = function (task, callback) {
        var file_path = res_dir + task.file_name;
        fs.exists(file_path, function (exists) {
            if(exists) {
                self.extract_file(file_path, callback);
            } else {
                var stream = ss.createStream();
                ss(socket).emit('file', stream, task.file_name);
                stream.pipe(fs.createWriteStream(task.file_name))
                    .on('finish', function () {
                        console.log('stream end.');
                        self.extract_file(file_path, callback);
                    })
                    .on('error', function () {
                        ss(socket).emit('file', stream, task.file_name); /// resent the request
                        console.log('error');
                    });
            }
        });
    };

    /**
     * prepare submission and test_setting
     * @param task
     * @param callback
     */
    this.pre_submission = function (task, callback) {
        child_process
            .spawn('python', ['./judge.py', 'prepare', self.id, self.tmpfs_size, self.cpu_mask, task.source_code, task.source_lang, task.test_setting], {stdio:'inherit'})
            .on('exit', function () {
                callback && callback();
            });
    };

    /**
     * prepare for juding
     * @param task
     * @param callback
     */

    this.prepare = function (task, callback) {
        self.change_status('preparing','Preparing for judging');
        async.parallel([
            function (callback) {
                self.pre_env(callback);
            },
            function (callback) {
                self.pre_submission(task, callback);
            },
            function (callback) {
                self.pre_file(task, callback);
            }
        ], callback);
    };

    /**
     * when everything is ready, it will start judging
     * @param task
     * @param callback
     */

    this.judge = function (task, callback) {
        self.change_status('judging', 'Judging');
        child_process
            .spawn('python', ['./judge.py', self.id, self.tmpfs_size, task.sumbit_code, task.submit_lang], {stdio:'inherit'})
            .on('exit', function () {
                callback && callback();
            });
    };

    /**
     * work on the task
     * @param task
     */

    this.work = function (task, callback) {
        async.series([
                //preparing
                function (callback) {
                    self.prepare(task, callback);
                },
                //judging
                function (callback) {
                    self.judge(task, callback);
                },
                //reporting
                function (callback) {
                    self.change_status('reporting', 'Reporting');
                    self.socket.emit('report', task, function () {
                        self.change_status('ready', 'Finished with ' + task + ' and ready now.');
                        callback && callback();
                    });
                }
            ],
            function (err) {
                if (err) {
                    self.log.write(err.message);
                }
                //console.log(result);
            }
        );

        // 以上需要严格串行
        callback && callback();
    };

    callback && callback();
}


module.exports = judge_client;




