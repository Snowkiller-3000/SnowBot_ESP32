class JoystickController {
  // stickID: ID of HTML element (representing joystick) that will be dragged
  // maxDistance: maximum amount joystick can move in any direction
  // deadzone: joystick must move at least this amount from origin to register value change
  constructor(stickID, maxDistance, deadzone) {
    this.id = stickID;
    let stick = document.getElementById(stickID);

    // location from which drag begins, used to calculate offsets
    this.dragStart = null;

    // track touch identifier in case multiple joysticks present
    this.touchId = null;

    this.active = false;
    this.value = { x: 0, y: 0 };

    let self = this;

    function handleDown(event) {
      self.active = true;

      // all drag movements are instantaneous
      stick.style.transition = "0s";

      // touch event fired before mouse event; prevent redundant mouse event from firing
      event.preventDefault();

      if (event.changedTouches)
        self.dragStart = {
          x: event.changedTouches[0].clientX,
          y: event.changedTouches[0].clientY,
        };
      else self.dragStart = { x: event.clientX, y: event.clientY };

      // if this is a touch event, keep track of which one
      if (event.changedTouches)
        self.touchId = event.changedTouches[0].identifier;
    }

    function handleMove(event) {
      if (!self.active) return;

      // if this is a touch event, make sure it is the right one
      // also handle multiple simultaneous touchmove events
      let touchmoveId = null;
      if (event.changedTouches) {
        for (let i = 0; i < event.changedTouches.length; i++) {
          if (self.touchId == event.changedTouches[i].identifier) {
            touchmoveId = i;
            event.clientX = event.changedTouches[i].clientX;
            event.clientY = event.changedTouches[i].clientY;
          }
        }

        if (touchmoveId == null) return;
      }

      const xDiff = event.clientX - self.dragStart.x;
      const yDiff = event.clientY - self.dragStart.y;
      const angle = Math.atan2(yDiff, xDiff);
      const distance = Math.min(maxDistance, Math.hypot(xDiff, yDiff));
      const xPosition = distance * Math.cos(angle);
      const yPosition = distance * Math.sin(angle);

      // move stick image to new position
      stick.style.transform = `translate3d(${xPosition}px, ${yPosition}px, 0px)`;

      // deadzone adjustment
      const distance2 =
        distance < deadzone
          ? 0
          : (maxDistance / (maxDistance - deadzone)) * (distance - deadzone);
      const xPosition2 = distance2 * Math.cos(angle);
      const yPosition2 = distance2 * Math.sin(angle);
      const xPercent = parseFloat((xPosition2 / maxDistance).toFixed(4));
      const yPercent = -1 * parseFloat((yPosition2 / maxDistance).toFixed(4));

      self.value = { x: xPercent, y: yPercent };
      doSend(
        "M" +
          JSON.stringify(joystick1.value.x) +
          "," +
          JSON.stringify(joystick1.value.y)
      ); // this sends the joystick position from the browser to the ESP32
    }

    function handleUp(event) {
      if (!self.active) return;

      // if this is a touch event, make sure it is the right one
      if (
        event.changedTouches &&
        self.touchId != event.changedTouches[0].identifier
      )
        return;

      // transition the joystick position back to center
      stick.style.transition = ".5s";
      stick.style.transform = `translate3d(0px, 0px, 0px)`;

      // reset everything
      self.value = { x: 0, y: 0 };
      self.touchId = null;
      self.active = false;
      doSend("STOP_M");
    }

    stick.addEventListener("mousedown", handleDown);
    stick.addEventListener("touchstart", handleDown);
    document.addEventListener("mousemove", handleMove, { passive: false });
    document.addEventListener("touchmove", handleMove, { passive: false });
    document.addEventListener("mouseup", handleUp);
    document.addEventListener("touchend", handleUp);
  }
}

var url = "ws://192.168.4.1:1024/";

let joystick1 = new JoystickController("stick1", window.innerWidth * 0.15, 0);

function loop() {
  requestAnimationFrame(loop);
}

// Sends a message to the server
function doSend(message) {
  websocket.send(message);
}

function onError(evt) {
  // nothing here lol
}

// Called when a message is received from the server
function onMessage(evt) {
  // the message is accessed by "evt.data"
  // switch(evt.data) {
  //     case "0":
  //
  //         break;
  //     case "1":
  //
  //         break;
  //     default:
  //         break;
  // }
}

// Called when a WebSocket connection is established with the server
function onOpen(evt) {
  doSend("hello");
}

// Called when the WebSocket connection is closed
function onClose(evt) {
  // Try to reconnect after a few seconds
  setTimeout(function () {
    wsConnect(url);
  }, 2000);
}

// Call this to connect to the WebSocket server
function wsConnect(url) {
  // Connect to WebSocket server
  websocket = new WebSocket(url);

  // Assign callbacks
  websocket.onopen = function (evt) {
    onOpen(evt);
  };
  websocket.onclose = function (evt) {
    onClose(evt);
  };
  websocket.onmessage = function (evt) {
    onMessage(evt);
  };
  websocket.onerror = function (evt) {
    onError(evt);
  };
}

// This is called when the page finishes loading
function init() {
  // Connect to WebSocket server
  wsConnect(url);
  horn_button.innerHTML = "Horn";
  arm_button.innerHTML = "Disarmed";
}

var horn_button = document.getElementById("horn_button");
var headlights_button = document.getElementById("headlights_button");
var LED_strip_button = document.getElementById("LED_strip_button");
var arm_button = document.getElementById("arm_button");

var up_button = document.getElementById("up_button");
var down_button = document.getElementById("down_button");
var left_button = document.getElementById("left_button");
var right_button = document.getElementById("right_button");

var headlights_button_state = 0;
var LED_strip_button_state = 0;
var arm_button_state = 0;

// Toggle Buttons

headlights_button.addEventListener("click", () => {
  headlights_button_state = !headlights_button_state;
  if (headlights_button_state) {
    headlights_button.style.color = "#ff6600";
    doSend("hl1");
  } else {
    headlights_button.style.color = "#ffffff";
    doSend("hl0");
  }
});

LED_strip_button.addEventListener("click", () => {
  LED_strip_button_state = !LED_strip_button_state;
  if (LED_strip_button_state) {
    LED_strip_button.style.color = "#ff6600";
    doSend("ls1");
  } else {
    LED_strip_button.style.color = "#ffffff";
    doSend("ls0");
  }
});

arm_button.addEventListener("click", () => {
  arm_button_state = !arm_button_state;
  if (arm_button_state) {
    arm_button.style.color = "#ff6600";
    arm_button.innerHTML = "Armed";
    doSend("arm1");
  } else {
    arm_button.style.color = "#ffffff";
    arm_button.innerHTML = "Disarmed";
    doSend("arm0");
  }
});

// Momentary Buttons

horn_button.addEventListener("mousedown", () => {
  horn_button.innerHTML = "HONK!";
  horn_button.style.color = "#ff6600";
  doSend("horn1");
});
horn_button.addEventListener("mouseup", () => {
  horn_button.innerHTML = "Horn";
  horn_button.style.color = "#ffffff";
  doSend("horn0");
});
horn_button.addEventListener("touchstart", () => {
  horn_button.innerHTML = "HONK!";
  horn_button.style.color = "#ff6600";
  doSend("horn1");
});
horn_button.addEventListener("touchend", () => {
  horn_button.innerHTML = "Horn";
  horn_button.style.color = "#ffffff";
  doSend("horn0");
});

up_button.addEventListener("mousedown", () => {
  up_button.style.color = "#ff6600";
  doSend("up1");
});
up_button.addEventListener("mouseup", () => {
  up_button.style.color = "#ffffff";
  doSend("up0");
});
up_button.addEventListener("touchstart", () => {
  up_button.style.color = "#ff6600";
  doSend("up1");
});
up_button.addEventListener("touchend", () => {
  up_button.style.color = "#ffffff";
  doSend("up0");
});

down_button.addEventListener("mousedown", () => {
  down_button.style.color = "#ff6600";
  doSend("down1");
});
down_button.addEventListener("mouseup", () => {
  down_button.style.color = "#ffffff";
  doSend("down0");
});
down_button.addEventListener("touchstart", () => {
  down_button.style.color = "#ff6600";
  doSend("down1");
});
down_button.addEventListener("touchend", () => {
  down_button.style.color = "#ffffff";
  doSend("down0");
});

left_button.addEventListener("mousedown", () => {
  left_button.style.color = "#ff6600";
  doSend("left1");
});
left_button.addEventListener("mouseup", () => {
  left_button.style.color = "#ffffff";
  doSend("left0");
});
left_button.addEventListener("touchstart", () => {
  left_button.style.color = "#ff6600";
  doSend("left1");
});
left_button.addEventListener("touchend", () => {
  left_button.style.color = "#ffffff";
  doSend("left0");
});

right_button.addEventListener("mousedown", () => {
  right_button.style.color = "#ff6600";
  doSend("right1");
});
right_button.addEventListener("mouseup", () => {
  right_button.style.color = "#ffffff";
  doSend("right0");
});
right_button.addEventListener("touchstart", () => {
  right_button.style.color = "#ff6600";
  doSend("right1");
});
right_button.addEventListener("touchend", () => {
  right_button.style.color = "#ffffff";
  doSend("right0");
});

window.addEventListener("load", init, false);

loop();
