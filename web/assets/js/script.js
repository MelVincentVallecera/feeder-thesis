/*  clock */
const hours = document.querySelector('.hours');
const minutes = document.querySelector('.minutes');
const seconds = document.querySelector('.seconds');

clock = () => {
    let today = new Date();
    let h = today.getHours() % 12 + today.getMinutes() / 59; // 22 % 12 = 10pm
    let m = today.getMinutes(); // 0 - 59
    let s = today.getSeconds(); // 0 - 59

    h *= 30; // 12 * 30 = 360deg
    m *= 6;
    s *= 6; // 60 * 6 = 360deg

    rotation(hours, h);
    rotation(minutes, m);
    rotation(seconds, s);

    // call every second
    setTimeout(clock, 500);
};

rotation = (target, val) => {
    target.style.transform = `rotate(${val}deg)`;
};

window.onload = clock();

function toggleDiv() {
    $('.components').toggle();
    $('.components2').toggle();
}

const firebaseConfig = {
    apiKey: "AIzaSyAkJnRiEeAwfSSLv7-2xdfUhTTKFrlVoXY",
    authDomain: "fish-feed-fe756.firebaseapp.com",
    databaseURL: "https://fish-feed-fe756-default-rtdb.asia-southeast1.firebasedatabase.app",
    projectId: "fish-feed-fe756",
    storageBucket: "fish-feed-fe756.appspot.com",
    messagingSenderId: "1012779793520",
    appId: "1:1012779793520:web:bef54e862ae82bbf91c8b5",
    measurementId: "G-SCS6J303SE"
};
firebase.initializeApp(firebaseConfig);

var countRef = firebase.database().ref('count');
countRef.on('value', function (snapshot) {
    count = snapshot.val()
    console.log(count)
});

function feednow() {
    firebase.database().ref().update({
        feednow: 1
    });
}

$(document).ready(function () {
    $('#timepicker').mdtimepicker(); //Initializes the time picker
    addDiv();
});

$('#timepicker').mdtimepicker().on('timechanged', function (e) {
    console.log(e.time)
    addStore(count, e);
    count = count + 1
    firebase.database().ref().update({
        count: parseInt(count),
    });
});

function addStore(count, e) {
    firebase.database().ref('timers/timer' + count).set({
        time: e.time
    });
    addDiv();
}

function showShort(id) {
    var idv = $(id)[0]['id']
    $("#time_" + idv).toggle();
    $("#short_" + idv).toggle();

}

function removeDiv(id) {
    var idv = $(id)[0]['id']
    firebase.database().ref('timers/' + idv).remove();
    if (count >= 0) {
        count = count - 1;
    }

    firebase.database().ref().update({
        count: parseInt(count),
    });
    $(id).fadeOut(1, 0).fadeTo(500, 0)
}

const waterTempElement = document.getElementById("waterTemp");
const ammoniaElement = document.getElementById("ammonia");
const doElement = document.getElementById("do");
const feedsElement = document.getElementById("feeds");
const phElement = document.getElementById("ph");
const waterLvlElement = document.getElementById("waterLevel");

var dbPath = 'sensors/';
var dbRef = firebase.database().ref(dbPath);
dbRef.on('value', snapshot => {
    var jsonData = snapshot.toJSON();
    var dissolvedOxygen = jsonData.dissolvedOxygen;
    var ammonia = jsonData.ammonia;
    var feeds = jsonData.feeds;
    var ph = jsonData.ph;
    var waterLevel = jsonData.waterLevel;
    var waterTemp = jsonData.waterTemp;

    waterTempElement.innerText = waterTemp;
    ammoniaElement.innerText = ammonia;
    doElement.innerText = dissolvedOxygen;
    feedsElement.innerText = feeds;
    phElement.innerText = ph;
    waterLvlElement.innerText = waterLevel;

});

function addDiv() {
    var divRef = firebase.database().ref('timers');
    divRef.on('value', function (snapshot) {
        var obj = snapshot.val()
        var i = 0;
        $('#wrapper').html('')
        while (i <= count) {
            var propertyValues = Object.entries(obj);
            let ts = propertyValues[i][1]['time']
            //var timeString = "12:04";
            var H = +ts.substr(0, 2);
            var h = (H % 12) || 12;
            h = (h < 10) ? ("0" + h) : h; // leading 0 at the left for 1 digit hours
            var ampm = H < 12 ? " AM" : " PM";
            ts = h + ts.substr(2, 3) + ampm;
            console.log(ts)

            const x = `
            <div id=${propertyValues[i][0]}>
                <div class="btn2 btn__secondary2" onclick=showShort(${propertyValues[i][0]}) id="main_${propertyValues[i][0]}">
                <div id="time_${propertyValues[i][0]}">
                ${ts}
                </div>
                <div class="icon2" id="short_${propertyValues[i][0]}" onclick=removeDiv(${propertyValues[i][0]})>
                    <div class="icon__add">
                        <ion-icon name="trash"></ion-icon>
                    </div>
                </div>
                </div>
                
                
            </div>`

            $('#wrapper').append(x);
            i++;
        }
    });
}