function updateTemperatureAndHumidityDisplay(temperature, humidity) {
    const trackTemperature  = document.getElementById("track-temperature");
    const trackHumidity     = document.getElementById("track-humidity");

    const maxTemperature = 50;
    const maxHumidity    = 100;

    const pathLengthTemperature = trackTemperature.getTotalLength();
    const pathLengthHumidity    = trackHumidity.getTotalLength();

    const valueLengthTemperature = pathLengthTemperature * ((maxTemperature - temperature) / maxTemperature);
    const valueLengthHumidity = pathLengthHumidity * ((maxHumidity - humidity) / maxHumidity);

    trackTemperature.getBoundingClientRect();
    trackHumidity.getBoundingClientRect();

    trackTemperature.style.strokeDashoffset = Math.max(0, valueLengthTemperature);
    trackHumidity.style.strokeDashoffset = Math.max(0, valueLengthHumidity);

    if (temperature <= 17) {
        trackTemperature.style.stroke = "#045DC2";
    } else if (temperature <= 34) {
        trackTemperature.style.stroke = "#01AA0F";
    } else {
        trackTemperature.style.stroke = "#C20404";
    }

    trackHumidity.style.stroke = "#0584EC";

    document.getElementById("value-temperature").innerHTML = temperature + " °C";
    document.getElementById("value-humidity").innerHTML = humidity + " %";
}

function main() {
    //const apiURL = window.location.href + "data";
    const apiURL = 'http://10.0.0.252/data';

    fetch(apiURL, {
        method: 'get',
        mode: 'no-cors'
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Error fetching data');
            }

            return response.json();
        })
        .then(data => {
            console.log('Received data: ', data);

            updateTemperatureAndHumidityDisplay(data.temperature, data.humidity);
        })
        .catch(error => {
            console.error('Error fetching data: ', error);
        });
}

setInterval(main, 1000);