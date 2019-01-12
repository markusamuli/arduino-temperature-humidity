# Arduino Temperature and Humidity Measuring Device

This is a small project created to display temperatures and humidities. The system uses Arduino Uno, DHT11 temperature sensor, LCD screen, switch and a potentiometer.

In the device there are two views, one displays the current temperatures and humidities, the other one displays the average measurements. The averages are calculated from the last 30 days, but if there are not enough measurements, only the valid ones are calculated. Average for one day is calculated by taking on measurement every hour and calculating their mean.

Arduino itself doesn't have a real time clock so while uploading the program the time needs to be synced to the board by giving it in 10 digit time format.

## Current temperature and humidity view
![img_0657](https://user-images.githubusercontent.com/25662645/51072265-8ed04580-1666-11e9-90f2-f6d9c35f266b.jpeg)

## Average temperatures and humidities view
![img_0658](https://user-images.githubusercontent.com/25662645/51072266-8ed04580-1666-11e9-9064-4bbc7783fae4.jpeg)
