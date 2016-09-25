#!/usr/bin/node
// version 7
//  - for new backend and authenticating queue setup with ttnctl

var mqtt=require('/usr/lib/node_modules/mqtt')
var mongodb=require('/usr/lib/node_modules/mongodb');
var request = require('/usr/lib/node_modules/request')

var mongodbClient=mongodb.MongoClient;
var mongodbURI='mongodb://user:password@192.168.1.75:27017/data'
var wurl = 'https://api.forecast.io/forecast/yourforecastioapikey/37.8270,-122.4230'
var weather = new Object();

function showit(topic,payload) {
  var obj = JSON.parse(payload.toString());
  console.log(obj['time']); }

function insertEvent(topic,payload) {
      mongodbClient.connect(mongodbURI, function(err,db) {
          if(err) { console.log(err); return; }
          else {
            // get the current humidity
            request({url:wurl,json:true}, function(error,response,body) {
              if (!error && response.statusCode === 200)
                 { weather=body }
              else
                 { return 1 }
              var obj = JSON.parse(payload.toString());
              console.log(obj['payload']);
              var coll = topic.split('/')[1]+'_'+topic.split('/')[2]+'_rh';
              collection = db.collection(coll);
              collection.insert(
                 { value:obj['payload'], humidity:weather['currently']['humidity'], when:new Date() },
                 function(err,docs) {
                    if(err) { console.log("Insert fail" + err); } // Improve error handling
                    else { console.log("Update callback - closing db");
                           db.close() }
              });  // end of insert block
            });   // end of request block
          }      // end of mongo  connect else block
      });       // end of mongo connect block
    }          // end of insertEvent



client=mqtt.connect('mqtt://staging.thethingsnetwork.org',{username:"putyourshere",password:"andyourshere"});
client.on('connect',function() {
 client.subscribe('yourapplicationid/devices/+/up');
});

client.on('message',insertEvent);

