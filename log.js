/**
 * Created by 明阳 on 2015/3/21.
 */

var fs = require('fs');

function log(path, callback) {
    this.path = path;
    var self = this;
    this.write = function (data, callback) {
        fs.appendFile(self.path, '[' + (new Date()).toLocaleString() + '] ' + data + '\n', callback);
    }
}

module.exports = log;