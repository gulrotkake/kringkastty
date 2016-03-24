window.onload = function() {
    var l = false;
    var loc = window.location;
    var uri = loc.protocol === "https:"
            ? "wss:"
            :  "ws:";
    uri += "//" + loc.host;

    var ws = new WebSocket(uri);
    var term;
    var status = document.getElementById('status');

    ws.onopen = function() {
        status.className='connected';
        status.innerHTML = 'Connected';
    };

    ws.onclose = function() {
        l = false;
        status.className='disconnected';
        status.innerHTML = 'Disconnected';
    };

    ws.onmessage = function(o) {
        if (!l) {
            l=true;
            var j = JSON.parse(o.data);
            term = new Terminal({
                cols: j.x,
                rows: j.y,
                writable : false,
                cursorBlink: false
            });
            term.open(document.getElementById('main'));
        } else {
            term.write(o.data);
        }
    };
};
