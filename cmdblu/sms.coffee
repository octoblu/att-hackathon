Meshblu = require './src/meshblu'
Device = require './src/device'
{spawn} = require 'child_process'
request = require 'request'
_ = require 'lodash'

device_uuid = process.env.DEVICE_UUID
device_token = process.env.DEVICE_TOKEN
payload_only = process.env.PAYLOAD_ONLY
meshblu_uri  = process.env.MESHBLU_URI || 'wss://meshblu.octoblu.com'

sendSMS = (message) =>
  console.log('Sending SMS')
  request.post
    uri: "https://api.sendhub.com/v1/messages/?username=4805706128&api_key=c067818f26b8d711672621f4dcec667ab86cdc48"
    json: 
      groups: [108260810164275910]
      text: message

debouncedSMS = _.debounce sendSMS, 1500

meshblu = new Meshblu device_uuid, device_token, meshblu_uri, =>
  console.log 'ready'
  meshblu.connection.subscribe(uuid: device_uuid)
  meshblu.onMessage (message) =>
    if payload_only
      console.log JSON.stringify(message.payload)
    else
      console.log JSON.stringify(message)

    debouncedSMS message.payload
