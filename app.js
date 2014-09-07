var skynet = require('skynet');
var uuids = {
  alcohol: '25944a21-3601-11e4-8e5a-919063640dc3',
  mike: 'ca03a096-b9e3-4c7c-8e77-b85cf7ad1d2f',
  koshin: 'effe7a0e-cb0f-4175-bc0e-3912749380d6',
  heart: 'd870d511-1c42-11e4-861d-89322229e557'
};

var heartInterval;

var conn = skynet.createConnection({
    "uuid": "80ac57c1-361c-11e4-acc2-497d40e3f98a",
    "token": "jgmi49lb7juoko6r081upb58jp7dgqfr"
});

conn.on('ready', function(data){
    var heartRate = 0;

    console.log('Ready');

    conn.on('message', function(data){
        console.log(data);
        if(data.fromUuid === uuids.heart) {
            setHeartRate(data.payload);
        }
    });

    conn.subscribe(uuids.heart, function(data){
        console.log('in subscribe');
    });

    conn.status(function (data) {
        console.log(data);
    });
});


function setHeartRate(heartRate) {
    console.log('saw heart');
    if(!heartRate) {
        switchBlink(false);
        return;
    }

    var milliseconds = ((60/heartRate) * 1000);

    if(heartInterval) {
        clearInterval(heartInterval);
    }

    heartInterval = setInterval(blinkBlink, milliseconds);
}


//expect bac to be between 0-1
function bac2Color(bac) {

    bac = bac * 255;
    var base = 0x00FFFF, color = base;

    //remove the 'blue' value the higher the bac is.
    color -= bac;
    //add the 'red' value the higher the bac is.
    color += (bac << 16);

    color = Math.round(color);

    color = color.toString(16);

    //zero-fill string
    while(color.length < 6) {
        color = '0' + color;
    }
    color = '#' + color;

    return color;
}

function blinkBlink() {
    switchBlink(true);
    setTimeout(function(){
        switchBlink(false);
    }, 100);
}

function switchBlink(on) {
    conn.message({
        devices: uuids.mike,
        subdevice: 'citi-blink',
        payload: {
            on: on,
            rgb: '#CC0000'
        }
    });
}