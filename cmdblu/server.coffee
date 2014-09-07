Meshblu = require './src/meshblu'
Device = require './src/device'
{spawn} = require 'child_process'
request = require 'request'

device_uuid = process.env.DEVICE_UUID
device_token = process.env.DEVICE_TOKEN
payload_only = process.env.PAYLOAD_ONLY
meshblu_uri  = process.env.MESHBLU_URI || 'wss://meshblu.octoblu.com'

meshblu = new Meshblu device_uuid, device_token, meshblu_uri, =>
  console.log 'ready'
  meshblu.connection.message({
    "devices": "feadee3e-7cb5-4fb5-93bd-1bcdba8de1c5",
    "subdevice" : "bean",
    "topic" : '',
    ack: 5,
    "payload": {
      "getAccelerometer":{}
    }
  });
  meshblu.onMessage (message) =>
    if payload_only
      console.log JSON.stringify(message.payload)
    else
      console.log JSON.stringify(message)

    try
      request.post(
        uri: "https://api.sendhub.com/v1/messages/?username=5306138494&api_key=996b0d7a9ebefde35e7a6a9b30e43f26749aa412"
        json: 
          groups: [173481]
          text: message.payload
      ,
        (err, resp) =>
          console.log err
          console.log resp
      )
    catch error
      console.log 'Error', error
