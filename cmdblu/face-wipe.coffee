Meshblu = require './src/meshblu'
Device = require './src/device'
{spawn} = require 'child_process'
request = require 'request'
_ = require 'lodash'

device_uuid = process.env.DEVICE_UUID
device_token = process.env.DEVICE_TOKEN
payload_only = process.env.PAYLOAD_ONLY
meshblu_uri  = process.env.MESHBLU_URI || 'wss://meshblu.octoblu.com'

send = (meshblu, messages) =>
  _.each messages, (msg) =>
    console.log 'Sending', JSON.stringify(msg)
    meshblu.connection.message msg

throttledSend = _.throttle send, 1000

heart_uuid  = 'd870d511-1c42-11e4-861d-89322229e557'

face_uuid   = '81d1fb81-36aa-11e4-8e5a-919063640dc3'
face_token = '00s2406pm0dbtvs4i6t0kjshknws714i'

blinky_uuid = '4c225891-1e4a-11e4-93b0-b35ccdc222f2'
rally_uuid  = 'c43462d1-1cea-11e4-861d-89322229e557/3c701ab0-2a69-11e4-ba29-b7d9779a4387'

meshblu = new Meshblu device_uuid, device_token, meshblu_uri, =>
  console.log 'ready'
  meshblu.connection.subscribe(uuid: device_uuid)

  meshblu.onMessage (message) =>
    if payload_only
      console.log JSON.stringify(message.payload)
    else
      console.log JSON.stringify(message)

    if message.payload.avg <= 0.3
      cmd = 'wipe'
      carMessage = 
        devices: [rally_uuid]
        payload: 
          m : cmd 

      throttledSend meshblu, [carMessage]

      setTimeout =>
        cmd = 'stopwipe'
        carMessage = 
          devices: [rally_uuid]
          payload: 
            m : cmd 

        throttledSend meshblu, [carMessage]
      , 5000
