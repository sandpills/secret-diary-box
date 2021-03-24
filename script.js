const serviceUuid = "19b10010-e8f2-537e-4f6c-d104768a1214";
let btn;
let myBLE;

let connectButton;
let textDiv;

//audio stuff
var gumStream; 						//stream from getUserMedia()
var rec; 							//Recorder.js object
var input; 							//MediaStreamAudioSourceNode we'll be recording

// shim for AudioContext when it's not avb. 
var AudioContext = window.AudioContext || window.webkitAudioContext;
var audioContext //audio context to help us record

var recordButton = document.getElementById("recordButton");
var stopButton = document.getElementById("stopButton");

var recording = false;
let currentValue;

//add events to those 2 buttons
recordButton.addEventListener("click", startRecording);
stopButton.addEventListener("click", stopRecording);


// event listener functions are initialized here:
function setup(event) {
  console.log("page is loaded");
  myBLE = new p5ble();
  // put the DOM elements into global variables:
  connectButton = document.getElementById("connect");
  connectButton.addEventListener("click", connectToBle);

  setInterval(setTime, 1000);
}

function connectToBle() {
  // Connect to a device by passing the service UUID
  myBLE.connect(serviceUuid, gotCharacteristics);
}

function gotCharacteristics(error, characteristics) {
  // if there's an error,
  // notify the user and quit the function:
  if (error) {
    console.log ("error: " + error);
  } else {
    let thisButton = connectButton;
    thisButton.value = "I'm connected!" ;
    btn = characteristics[0];
    myBLE.read(btn, gotValue);
  }
}

// This function will be called once you have a value:
function gotValue(error, value) {

    let thisSpan = document.getElementById("statetext");
    let thisTxt = document.getElementById("messages");
    if (error) console.log('error: ', error);
    // After getting a value, call p5ble.read() again to get the value again
    myBLE.read(btn, gotValue);
    console.log(value);
    if (currentValue != 1){
      recording = true;
      currentValue = value;
    } else if (currentValue == 1){
      recording = false;
      currentValue = value;
    }
    if (value == 1) {
      if (recording) {
        //start recording
        startRecording();
        console.log("start recording");
        recording = false;
      }
    } else {
      if (!recording) {
        //stop recording
        stopRecording();
        console.log("stop recording");
      }   
    }
  }

//time
  function setTime() {
    let now = new Date().toTimeString();
    let timeSpan = document.getElementById('time');
    timeSpan.innerHTML = now;
  }

//recorder
function startRecording() {
	console.log("recordButton clicked");

	/*
		Simple constraints object, for more advanced audio features see
		https://addpipe.com/blog/audio-constraints-getusermedia/
	*/
    
    var constraints = { audio: true, video:false }

	recordButton.disabled = true;
	stopButton.disabled = false;

	/*
    	We're using the standard promise based getUserMedia() 
    	https://developer.mozilla.org/en-US/docs/Web/API/MediaDevices/getUserMedia
	*/

	navigator.mediaDevices.getUserMedia(constraints).then(function(stream) {
		console.log("getUserMedia() success, stream created, initializing Recorder.js ...");

		/*
			create an audio context after getUserMedia is called
			sampleRate might change after getUserMedia is called, like it does on macOS when recording through AirPods
			the sampleRate defaults to the one set in your OS for your playback device

		*/
		audioContext = new AudioContext();

		/*  assign to gumStream for later use  */
		gumStream = stream;
		/* use the stream */
		input = audioContext.createMediaStreamSource(stream);

		/* 
			mono or dual channels
		*/
		rec = new Recorder(input,{numChannels:2})

		//start the recording process
		rec.record();
		console.log("Recording started");

	}).catch(function(err) {
	  	//enable the record button if getUserMedia() fails
    	recordButton.disabled = false;
    	stopButton.disabled = true;
	});
}

function stopRecording() {
	console.log("stopButton clicked");

	//disable the stop button, enable the record too allow for new recordings
	stopButton.disabled = true;
	recordButton.disabled = false;

	//tell the recorder to stop the recording
	rec.stop();

	//stop microphone access
	gumStream.getAudioTracks()[0].stop();

	//create the wav blob and pass it on to createDownloadLink
	rec.exportWAV(createDownloadLink);
}

function createDownloadLink(blob) {
	
	var url = URL.createObjectURL(blob);
	var au = document.createElement('audio');
	var li = document.createElement('li');
	var link = document.createElement('a');

	//name of .wav file to use during upload and download (without extendion)
	var filename = new Date().toISOString();
		
	//recording time
	li.appendChild(document.createTextNode(new Date().toString()));
	li.appendChild(document.createElement("br"));
	li.appendChild(document.createElement("br"));

	//add controls to the <audio> element
	au.controls = true;
	au.src = url;

	//save to disk link
	link.href = url;
	link.download = filename+".wav"; //download forces the browser to donwload the file using the  filename
	link.innerHTML = "download";

	//add the new audio element to li
	li.appendChild(au);

	//add the save to disk link to li
	li.appendChild(link);
	li.appendChild(document.createTextNode (" "))//add a space in between
	
	//upload link
	var upload = document.createElement('a');
	upload.href="#";
	upload.innerHTML = "upload";
	upload.addEventListener("click", function(event){
		  var xhr=new XMLHttpRequest();
		  xhr.onload=function(e) {
		      if(this.readyState === 4) {
		          console.log("Server returned: ",e.target.responseText);
		      }
		  };
		  var fd=new FormData();
		  fd.append("audio_data",blob, filename);
		  xhr.open("POST","upload.php",true);
		  xhr.send(fd);
	})
	li.appendChild(document.createTextNode (" "))//add a space in between
	li.appendChild(upload)//add the upload link to li

	//add the li element to the ol
	recordingsList.appendChild(li);
}

// This is a listener for the page to load.
// This is the command that actually starts the script:
window.addEventListener("DOMContentLoaded", setup);
