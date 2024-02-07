    const devicesSocket = new ReconnectingWebSocket(
        'ws://'
        + window.location.host
        + '/ws/devices/'
    );
    var timeoutTimer = null;
    function setup_ws()
    {
        devicesSocket.onmessage = function(e) {
            const data = JSON.parse(e.data);
            console.log(data);
            if("rf_outlet_status" in data)
            {
                var outlet = data.rf_outlet_status;
                var outlet_str = (outlet.is_on ? 'On' : 'Off');
                document.getElementById("rf_outlet_state_" + outlet.id).textContent = outlet_str;
            }
            else if("rgb_light_status" in data)
            {
                var light = data.rgb_light_status;
                var redhex = ('00' + light.red.toString(16).toUpperCase()).slice(-2);
                var greenhex = ('00' + light.green.toString(16).toUpperCase()).slice(-2);
                var bluehex = ('00' + light.blue.toString(16).toUpperCase()).slice(-2);
                document.getElementById("rgb-color-input-" + light.unique_id).value = "#" + redhex + greenhex + bluehex;
                document.getElementById("rgb-color-button-" + light.unique_id).innerHTML = (light.is_on ? 'On' : 'Off');
            }
            else if("mqtt_garage_update" in data)
            {
                console.log("GOT GARAGE UPDATE!");
                console.log(data);
		clearTimeout(timeoutTimer);
                document.getElementById("garage-status").textContent = "Garage Status: " + data.mqtt_garage_update.status;
            }
        };
    }

    devicesSocket.onclose = function(e) {
        console.error('Devices socket closed unexpectedly');
        var div = document.getElementById( 'main-div' );
        div.classList.remove("w3-teal");
        div.classList.add("w3-red");
    };

    devicesSocket.onopen = function(e) {
        console.error('Devices socket closed unexpectedly');
        var div = document.getElementById( 'main-div' );
        div.classList.remove("w3-red");
        div.classList.add("w3-teal");
    };

    function toggle_rf_outlet(rf_outlet_id) {
        console.log("Sending: " + rf_outlet_id);
        devicesSocket.send(JSON.stringify({'rf_outlet_toggle': rf_outlet_id}));
    }
    function toggle_rgb_light(rgb_light_id) {
        console.log("Sending: RGB Light ID = " + rgb_light_id);
        devicesSocket.send(JSON.stringify({'rgb_light_toggle': rgb_light_id}));
    }
    function update_rgb_light_color(rgb_light_id, color) {
        console.log("Sending: RGB Light Color to ID = " + rgb_light_id);
        devicesSocket.send(JSON.stringify({'rgb_light_color_update': {"id": rgb_light_id, "hexcolor": color}}));
    }
    function no_response_from_esp() {
	document.getElementById("garage-status").textContent = "Garage Status: No response from sending unit.";
    }
    function open_garage_door() {
        let isExecuted = confirm("Are you sure to open garage door?");
        if(isExecuted)
        {
            console.log("Sending: Open Garage Door");
            document.getElementById("garage-status").textContent = "Garage Status: Sending Request";
	    timeoutTimer = setTimeout(no_response_from_esp, 10000);
            devicesSocket.send(JSON.stringify({'open_garage_door': {} }));
        }
    }
    function close_garage_door() {
        document.getElementById("garage-status").textContent = "Garage Status: Sending Request";
	timeoutTimer = setTimeout(no_response_from_esp, 10000);
        console.log("Sending: Close Garage Door");
        devicesSocket.send(JSON.stringify({'close_garage_door': {} }));
    }
    function query_garage_door() {
        console.log("Sending: Garage Door Query");
        document.getElementById("garage-status").textContent = "Garage Status: Sending Request";
	timeoutTimer = setTimeout(no_response_from_esp, 10000);
        devicesSocket.send(JSON.stringify({'query_garage_door': {} }));
    }
    function send_color(unique_id)
    {
        var color = document.getElementById("rgb-color-input-" + unique_id).value;
        console.log("Sending color: " + color + " to id: " + unique_id);
        update_rgb_light_color(unique_id, color);
    }
