/**
 * Created by lmy on 15-3-26.
 */

require('daemon')();

var judge_client = require('./judge_client')
var config = require('./config');
var cluster = require('cluster');


/**
 * create one worker form config
 * @param config
 */
function createWorker(config) {
    if (cluster.isMaster) {
        // Fork a worker if running as cluster master
        var child = cluster.fork();

        // Respawn the child process after exit
        // (ex. in case of an uncaught exception)
        child.on('exit', function (code, signal) {
            createWorker(config);
        });
    } else {
        // Run the HTTP server if running as worker
        var judge = new judge_client(config);
        judge.start();
    }
}

/**
 * create all workers form configs
 * @param configs
 */
function createWorkers(configs) {
    configs.forEach(createWorker);
}

/**
 * kill all workers with the given signal.
 * also removes all event listeners form workers before sending the signal
 * to prevent spawning again
 * @param signal
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

/**
 * Restart the workers
 */
process.on('SIGNUP', function () {
    killAllWorkers('SIGTERM');
    createWorkers(config);
});

/**
 * Gracefully Shuts down the workers
 */
process.on('SIGTERM', function () {
    killAllWorkers('SIGTERM');
});

createWorkers(config);