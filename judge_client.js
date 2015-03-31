var io = require('socket.io-client');
var crypto = require('crypto');
var async = require('async');
var log = require('./log');
var child_process = require('child_process');


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
    /**
     * start the process
     */
    this.start = function () {
        self.socket = io.connect(self.url)
            .on('connect', function () {
                self.log.write('connected with ' + self.config);
                self.status = 'ready';
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
                //self.log_in();
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
        child_process
            .spawn('python', ['./judge.py', 'mount', self.id, self.tmpfs_size, self.cpu_mask], {stdio:'inherit'})
            .on('exit', function () {
                callback && callback();
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
     * prepare files for judging
     * @param task
     * @param callback
     */

    this.pre_file = function (task, callback) {
        //TODO: 文件准备同时把测试设置写入环境
        setTimeout(function () {
            console.log("preparing files");
            callback && callback();
        }, 1000 + Math.random() * 100);
    };

    /**
     * prepare for juding
     * @param task
     * @param callback
     */

    this.prepare = function (task, callback) {
        self.status = 'preparing';
        self.socket.emit('status', self.status, function () {
            self.log.write('Preparing for judging');
        });
        async.parallel([
            function (callback) {
                self.pre_env(callback);
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
        self.status = 'judging';
        self.socket.emit('status', self.status, function () {
            self.log.write('Judging');
        });

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

    this.work = function (task) {
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
                    self.status = 'reporting';
                    self.socket.emit('report', self.status, task, function () {
                        self.log.write('Finished with ' + task);
                        self.status = 'ready';
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
    };

    callback && callback();
}


module.exports = judge_client;




