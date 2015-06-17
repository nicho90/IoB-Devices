var express = require('express');
var path = require('path');
var logger = require('morgan');
var cookieParser = require('cookie-parser');
var bodyParser = require('body-parser');
var debug = require('debug');
var pg = require('pg');

// WEBSERVER
var app = express();
app.set('port', process.env.PORT || 5000);

var server = app.listen(app.get('port'), function() {
    debug('Express server listening on port ' + server.address().port);
    console.log('Express server listening on port ' + server.address().port);
});

app.use(logger('dev'));
app.use(bodyParser.json());
app.use(bodyParser.urlencoded());
app.use(cookieParser());


// DATABASE
var conString = "postgres://Nicho:@localhost/iob";

var client = new pg.Client(conString);
client.connect(function(err) {
  if(err) {
    return console.error('could not connect to postgres', err);
  }
  client.query('SELECT NOW() AS "theTime"', function(err, result) {
    if(err) {
      return console.error('error running query', err);
    }
    console.log("Database online: " + result.rows[0].theTime);
    //client.end();
  });
});


// REST-API
// Insert-Request
app.get('/insert', function(req, res) {

    var data = {};
    data.deviceID = req.query.id;
    data.snr = Number(JSON.parse(req.query.snr));
    data.rssi = Number(JSON.parse(req.query.rssi));
    data.time = req.query.time;
    data.station = req.query.station;
    data.avgSignal = Number(JSON.parse(req.query.avgSignal));
    data.duplicate = Boolean(JSON.parse(req.query.duplicate));


    // PROCESSING data-String
    var _data = req.query.data;

    // Status theftprotection
    data.theftprotection = Boolean(JSON.parse(_data.substring(1, 2)));

    var _mode = _data.substring(18, 20);
    data.mode = Buffer(_mode, 'hex').readIntLE(0);

    // Longitude
    var subStr_1 = _data.substring(2, 4);
    var subStr_2 = _data.substring(4, 6);
    var subStr_3 = _data.substring(6, 8);
    var subStr_4 = _data.substring(8, 10);
    var lng = subStr_4.concat(subStr_3).concat(subStr_2).concat(subStr_1);
    data.lng = Buffer(lng, 'hex').readFloatBE(0);

    // Longitude
    var subStr_5 = _data.substring(10, 12);
    var subStr_6 = _data.substring(12, 14);
    var subStr_7 = _data.substring(14, 16);
    var subStr_8 = _data.substring(16, 18);
    var lat = subStr_8.concat(subStr_7).concat(subStr_6).concat(subStr_5);
    data.lat = Buffer(lat, 'hex').readFloatBE(0);


    // Temperature
    /*var subStr_9 = _data.substring(18,20);
    var subStr_10 = _data.substring(20,22);
    var subStr_11 = _data.substring(22,24);
    var subStr_12 = _data.substring(24,26);
    var lat = subStr_12.concat(subStr_11).concat(subStr_10).concat(subStr_9);
    data.lat = Buffer(lat, 'hex').readFloatBE(0);*/

    if (data.mode === 1) {

        /*
            ...
        */

        client.query("INSERT INTO devices (name) VALUES ('" + data.deviceID + "');", function(err, result) {
            if(err) {
                console.error('error running query');
                done();
                return next(new Error('Database error'));
            } else {

                console.log(result);

                // SEND Result
                res.status(200).json(data);
            }
        });
    } else if (data.mode === 2) {

        /*
            ...
        */
        res.status(200).json(data);
    } else {
        res.status(200).send('Error: Property "data.mode" missing!');
    }

});

// Measurements-Request
app.get('/measurements', function(req, res) {

    var data = [{
        "id": "1",
        "deviceID": "15E2C",
        "snr": 7.57,
        "rssi": -105.5,
        "time": "1434401065",
        "station": "1D3C",
        "avgSignal": 27.83,
        "duplicate": false,
        "theftprotection": true,
        "lng": 51.95978546142578,
        "lat": 7.646671772003174
    }, {
        "id": "2",
        "deviceID": "15E2C",
        "snr": 7.59,
        "rssi": -104.4,
        "time": "1434401089",
        "station": "1D3C",
        "avgSignal": 39.21,
        "duplicate": false,
        "theftprotection": false,
        "lng": 51.95978546142578,
        "lat": 7.646671772003174
    }];

    // SEND Result
    res.status(200).json(data);
});


/*
    {"deviceID":"15E2H","snr":7.57,"rssi":-105.5,"time":"1434401065","station":"1D3C","avgSignal":27.83,"duplicate":false,"theftprotection":true,"mode":10,"lng":51.95978546142578,"lat":7.646671772003174}
*/
