# Mini Project 2022
Mini-Project

Este miniproyecto es un balancín que busca automáticamente la posición de equilibrio.
This project is about a self balanced machine using an ESP32 board to coordinate, manage the control system and host a webserver responsible for the GUI using a webpage.

To do list
A. Create PID controller
	a. The value of reference: 0.5 if normalized sensor, around 9.8 if not normalized
	b. The normalized sensor range is -5 to +5
	c. The equilibrium position is not 0, it is around 0.5
B. Create a pretty webpage
	a. Display general information of the sensor readings
		i. Aceleration in all axis
		ii. Angular speed
	b. Make a plot for the inclination
	c. Make an animation (optional)
		i. Use the reference model found
C. Optimize for speed
	a. Use timers for the pid control to guarantee a constant fs
	b. Keep LED indicator for users in operation mode
