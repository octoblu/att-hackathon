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

sendSMS = =>
  console.log('Sending SMS')
  request.post
    uri: "https://api.sendhub.com/v1/messages/?username=5306138494&api_key=996b0d7a9ebefde35e7a6a9b30e43f26749aa412"
    json: 
      groups: [173481]
      text: 'RESCUE ME!'

deboucedSend = _.throttle send, 1000
debouncedSMS = _.once sendSMS

face_uuid = '81d1fb81-36aa-11e4-8e5a-919063640dc3'
blinky_uuid = '4c225891-1e4a-11e4-93b0-b35ccdc222f2'
rally_uuid = 'c43462d1-1cea-11e4-861d-89322229e557/3c701ab0-2a69-11e4-ba29-b7d9779a4387'

meshblu = new Meshblu device_uuid, device_token, meshblu_uri, =>
  console.log 'ready'
  meshblu.connection.subscribe(uuid: device_uuid)

  meshblu.onMessage (message) =>
    if payload_only
      console.log JSON.stringify(message.payload)
    else
      console.log JSON.stringify(message)

    val = message.payload[1] - 500
    if val < 0
      val = 0

    val = (val / 50)*2
    if val > 9
      val = 9

    bac_level = parseInt(val) || 0

    messages = []
    blinkyMessage = {
      devices: [blinky_uuid],
      payload: bac_level
    }
    messages.push blinkyMessage

    if bac_level > 4
      carMessage = {
        devices: [rally_uuid],
        payload: 
          m : 'lock'
      }
      messages.push carMessage
      debouncedSMS()

    deboucedSend(meshblu, messages);
