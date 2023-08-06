    const devicesSocket = new WebSocket(
        'ws://'
        + window.location.host
        + '/ws/devices/'
    );
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
                var light_str = (light.is_on ? 'On, ' : 'Off, ') + "red=" + light.red + ", green=" + light.green
                    + ", blue=" + light.blue;
                document.getElementById("rgb_light_state_" + light.id).textContent = light_str;
            }
            else if("mqtt_garage_update" in data)
            {
                console.log("GOT GARAGE UPDATE!");
                console.log(data);
                document.getElementById("garage-status").textContent = "Garage Status: " + data.mqtt_garage_update.status;
            }
        };
    }

    function check_ws()
    {
        if (devicesSocket.readyState === WebSocket.CLOSED) {
            show_refresh_popup();
        }
    }

    function show_refresh_popup()
    {
        let isExecuted = confirm("Websocket Connection Lost: Refresh?");
        if(isExecuted)
        {
            location.reload();
        }
    }

    devicesSocket.onclose = function(e) {
        console.error('Devices socket closed unexpectedly');
        var div = document.getElementById( 'main-div' );
        div.classList.remove("w3-teal");
        div.classList.add("w3-red");
        const myTimeout = setTimeout(show_refresh_popup, 100);
    };

    function toggle_rf_outlet(rf_outlet_id) {
        check_ws();
        console.log("Sending: " + rf_outlet_id);
        devicesSocket.send(JSON.stringify({'rf_outlet_toggle': rf_outlet_id}));
    }
    function toggle_rgb_light(rgb_light_id) {
        check_ws();
        console.log("Sending: RGB Light ID = " + rgb_light_id);
        devicesSocket.send(JSON.stringify({'rgb_light_toggle': rgb_light_id}));
    }
    function open_garage_door() {
        check_ws();
        let isExecuted = confirm("Are you sure to open garage door?");
        if(isExecuted)
        {
            console.log("Sending: Open Garage Door");
            document.getElementById("garage-status").textContent = "Garage Status: Sending Request";
            devicesSocket.send(JSON.stringify({'open_garage_door': {} }));
        }
    }
    function close_garage_door() {
        check_ws();
        document.getElementById("garage-status").textContent = "Garage Status: Sending Request";
        console.log("Sending: Close Garage Door");
        devicesSocket.send(JSON.stringify({'close_garage_door': {} }));
    }
    function query_garage_door() {
        check_ws();
        console.log("Sending: Garage Door Query");
        document.getElementById("garage-status").textContent = "Garage Status: Sending Request";
        devicesSocket.send(JSON.stringify({'query_garage_door': {} }));
    }