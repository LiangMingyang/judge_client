/**
 * Created by lmy on 15-3-26.
 */


var judge_client = require('./judge_client_new');
var config = require('./config');
var cluster = require('cluster');
var fs = require('fs');
var PIDFILE = 'judge_controller.pid';

/**
 * create one worker form config
 * @param config {Object} the config to every judge_client
 */
function createWorker(config) {
    // Fork a worker if running as cluster master
    var child = cluster.fork();

    // Respawn the child process after exit but the child won't be deleted and the number of it will increase...
    // (ex. in case of an uncaught exception)
    child.send(config);
    child.on('exit', function (code) {
        console.log(child.id + 'exited' + 'with code:'+code);
        createWorker(config);
        console.log('restart with'+config.name);
    });
}

/**
 * create all workers form configs
 * @param configs {Object} configs of judge_client
 */
function createWorkers(configs) {
    configs.forEach(createWorker);
}

/**
 * kill all workers with the given signal.
 * also removes all event listeners form workers before sending the signal
 * to prevent spawning again
 * @param signal {String} signal of kill
 */
function killAllWorkers(signal) {
    var uniqueID,
        worker;

    for (uniqueID in cluster.workers) {
        if (cluster.workers.hasOwnProperty(uniqueID)) {
            worker = cluster.workers[uniqueID];
            worker.removeAllListeners();
            worker.process.kill(signal);
        }
    }
}


if(cluster.isMaster) {
    /**
     * must run as root
     * first step: write process.pid to PIDFILE
     * second step: create workers
     */
    //noinspection JSUnresolvedVariable,JSUnresolvedFunction
    if (process.hasOwnProperty('getgid') && process.getgid() !== 0) {
        console.log('Please run me as root');
        return;
    }
    //noinspection JSUnresolvedVariable
    /**
    * Restart the workers
    */
    process.on('SIGHUP', function () {
        console.log('Got SIGHUP');
        killAllWorkers('SIGTERM');
        createWorkers(config);
    });

    //noinspection JSUnresolvedVariable
    /**
    * Gracefully Shuts down the workers
    */
    process.on('SIGTERM', function () {
        killAllWorkers('SIGTERM');
        fs.unlink(PIDFILE, function (err) {
            if(err) {
                console.log(err.message);
                return ;
            }
            console.log('Stopped running');
        });
    });

    //noinspection JSUnresolvedVariable
    fs.writeFileSync(PIDFILE, process.pid);
    createWorkers(config);

} else {
    //noinspection JSUnresolvedVariable
    /**
     * worker will get its config
     */
    process.on('message', function (config) {
        console.log('created a worker with ', config);
        var judge = new judge_client(config);
        judge.init(function () {
            console.log(config.name+'running now');
        });
    });
}

