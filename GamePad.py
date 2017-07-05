import serial
from serial.tools.list_ports import comports
from tkinter import *

class GamePad:
	def __init__(self, window, ser):
		self.ser = ser
		trigger_button = Button(window, text="NOW", command=self.trigger, height=5, width=20)
		trigger_button.place(relx=0.05, rely=0.05, relwidth=0.9, relheight=0.9)

		window.bind('<space>', self.trigger)
		window.bind('<Return>', self.trigger)
		
		for letter in [chr(c) for c in range(ord('a'), ord('z') + 1)]:
			window.bind('<' + letter + '>', self.trigger)
		
		for number in range(10):
			window.bind('<' + str(number) + '>', self.trigger)
				

	def trigger(self, *args):
		self.ser.write(b' ')


def main(ser):
	window = Tk()
	window.title("Game Pad")
	window.minsize(300, 300)
	GamePad(window, ser)
	window.mainloop()


if __name__ == "__main__":
	available_ports = comports()
	print("Available ports:")
	for available_port in available_ports:
		print(available_port)
	
	PORT_NAME = input("Please enter a valid port name (e.g. \"COM3\"):")
	ser = serial.Serial(PORT_NAME, 9600)
		
	main(ser)
	ser.close()
