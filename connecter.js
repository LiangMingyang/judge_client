/**
 * Created by lmy on 15-3-28.
 */


var daemon = require('daemon');
var commander = require('commander');
var fs = require('fs');
var PIDFILE = 'judge_controller.pid';
var child_process = require('child_process');
const os = require('os');

function stop() {
    fs.readFile(PIDFILE, 'utf-8', function (err, PID) {
        if(err) {
            console.log('Can not stop it! It may be not running.');
            console.log(err.message);
            return;
        }
        console.log('The pid is ' + PID);
        if(os.platform() == 'win32') {
            child_process.spawn('tskill',[PID], {stdio:'inherit'}).on('exit', function () {
                fs.unlink(PIDFILE);
                console.log('killed');
            });
        } else
        child_process.spawn('kill',['-TERM',PID], {stdio:'inherit'}).on('exit', function () {
            console.log('killed');
        });
    });
}

function start() {
    console.log('starting');
    if(!commander.force && fs.existsSync(PIDFILE)) {
        console.log('But it is already running');
        return ;
    }
    if(commander.no_daemon) {
        child_process.spawn('node',['./judge_controller.js'],{stdio:'inherit'});
    } else {
        daemon.daemon('./judge_controller.js', []);
    }
}

// function restart() {
//     fs.readFile(PIDFILE, 'utf-8', function (err, PID) {
//         if(err) {
//             console.log('Can not restart it! It may be not running.');
//             console.log(err.message);
//             return;
//         }
//         console.log(PID);
//         child_process.spawn('kill',['-HUP',PID]).on('exit', function () {
//             console.log('restarted');
//         });
//     });
// }

commander
    .version('1.0.0')
    .usage('[command] [options]');

commander
    .command('start')
    .description('program will start')
    .action(start);

commander
    .command('stop')
    .description('program will stop')
    .action(stop);


commander
    .option('-n --no_daemon', 'if it will run as daemon')
    .option('-f --force', 'force start')
    .parse(process.argv);