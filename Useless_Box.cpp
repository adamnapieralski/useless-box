/*
Author: Adam Napieralski

USELESS BOX with light following mode
*/


#include <basicTB6612FNG.h>
#include <Servo.h>

#define servo_pin 10
Servo hand_servo;

#define left_PWM_pin 5 //silnik A
#define left_FWD_pin 2
#define left_REV_pin 3
#define right_PWM_pin 11 //silnik B
#define right_FWD_pin 12
#define right_REV_pin 8

Motor left_track(left_PWM_pin, left_FWD_pin, left_REV_pin);
Motor right_track(right_PWM_pin, right_FWD_pin, right_REV_pin);

#define normalSpeed 225 //predkosc stala swiatloluba
#define rotationTime 2100 //czas potrzebny do zrobienia pelnego obrotu

#define trigPin 4 //czujnik odleg?osci
#define echoPin 6

#define HandDist 9 //maksymalne wychwytywane zblizenie reki (cm)

#define stdbPin A0 //pin od do ustawiania stanu czuwania na silnikach

#define builtInLed 13 //dioda wbudowana w arduino (na pinie 13)
#define buzzPin 9 //buzzer

#define mainOnOffPin 7 //przycisk wylaczajacy
#define UB_switchPin A4 //przelacznik glowny (do odklikiwania)
#define mode_switchPin A5 //przelacznik trybu: UB/swiatlolub*)


#define rightFotoPin A1 //prawy fotorezystor
#define leftFotoPin A2 //lewy fotorezytor

#define basic_servo_posit 0
#define door_servo_posit 19
#define max_servo_posit 121
#define touch_servo_posit 111

#define FRWD 1 //do funkcji Move
#define RVRS 0

#define RIGHT 1 //do funkcji rotation
#define LEFT 0 

#define deltaFotoTrig 35 //roznica w odczytach z fotorezystorow, ktora powoduje ruch

int rightFotoVal = 0; //wartosci odczytane z fotorezystorow
int leftFotoVal = 0;

bool mainOnOff = true; //czy robot jest wlaczony czy nie (obslugiawny przyciskiem)
bool swiatlolubOnOff = true; //czy swiatlolub jest wlaczony (czujnikiem zblizenia - reka)

const byte movesNumber = 12; //ilosc zaprogramowanych ruchow

unsigned long timer = 0, timer2 = 0;
int backingCount = 0;
int randNum = 0;
bool completedMoves[movesNumber];

void setup()
{
	randomSeed(analogRead(A3));
	Serial.begin(9600);
	hand_servo.attach(servo_pin);
	hand_servo.write(basic_servo_posit);
	Serial.println(hand_servo.read());

	pinMode(trigPin, OUTPUT);
	pinMode(echoPin, INPUT);

	pinMode(builtInLed, OUTPUT); //wbudowana dioda
	digitalWrite(builtInLed, HIGH); //swieci sie, bo dziala

	pinMode(buzzPin, OUTPUT); //buzzer

	pinMode(stdbPin, OUTPUT); //tryb standby na silnikach
	StandByOn();

	pinMode(UB_switchPin, INPUT_PULLUP); //przelacznik Useless Boxa
	pinMode(mode_switchPin, INPUT_PULLUP); //przelacznik trybow
	pinMode(mainOnOffPin, INPUT_PULLUP); //glowny przycisk

	for (int i = 0; i < movesNumber; i++)
		completedMoves[i] = false;

	left_track.ChangeSpeed(255);
	right_track.ChangeSpeed(255);

	left_track.ChangeDirection(FORWARD);
	right_track.ChangeDirection(FORWARD);
}

void loop()
{
	/*sprawdzenie stanu glownego wlacznika*/
	if (digitalRead(mainOnOffPin) == LOW)
	{
		while (digitalRead(mainOnOffPin) == LOW)
		{
		}
		if (mainOnOff == true)
		{
			mainOnOff = false; digitalWrite(builtInLed, LOW); Buzz(700);
			left_track.Stop(); right_track.Stop();
		}
		else
		{
			mainOnOff = true; digitalWrite(builtInLed, HIGH); Buzz(700);
		}
	}
	if (mainOnOff == true)
	{
		/*tryb Useless Boxa*/
		if (digitalRead(mode_switchPin) == HIGH)
		{
			left_track.Stop(); right_track.Stop();
			left_track.ChangeSpeed(255); right_track.ChangeSpeed(255);

			/*losowanie numeru ruchu az wylosuje sie ruch, ktorego nie bylo wczesniej*/
			randNum = random(1, movesNumber);
			while (completedMoves[randNum - 1] == true)
				randNum = random(1, movesNumber);
			/*bedziemy wykonywac ten ruch, wiec zapisujemy go do tablicy jako true*/
			completedMoves[randNum - 1] = true;
			/*sprawdzenie czy wszystkie ruchu juz wystapily*/
			int temp = 0;
			for (int i = 0; i < movesNumber; i++)
			{
				if (completedMoves[i] == true) temp++;
			}
			Serial.print("Temp"); Serial.print(temp); Serial.println();
			/*jesli tak to zaczynamy losowanie dla wszystkich od poczatku*/
			if (temp == movesNumber)
			{
				for (int j = 0; j < 12; j++) completedMoves[j] = false;
			}

			Serial.println(randNum);

			/*realizacja wylosowanego ruchu*/
			switch (randNum)
			{
			case 1:
				/*czekanie na pojawienie sie reki*/
				while ((HandCheck() == false) && (digitalRead(mode_switchPin) == HIGH)) {}
				if (digitalRead(mode_switchPin) == HIGH)
				{
					/*uciekanie, kiedy pojawi sie reka*/
					HandRunBack();
					timer = millis();
					while (millis() < timer + 5000)
					{
						HandRunBack();
					}
					Move(500, FRWD);
				}
				break;

			default:
				/*czekanie na przelaczenie przelacznika UB lub wylacznika glownego*/
				while ((digitalRead(UB_switchPin) == HIGH) && (digitalRead(mode_switchPin) == HIGH)) {}
				/*jesli przelaczono UB*/
				if (digitalRead(UB_switchPin) == LOW)
				{
					switch (randNum)
					{
					case 2: UltraSlowSwitch(); break;
					case 3: SlowAppear(); break;
					case 4: SlowDisappear(); break;
					case 5: BackingNormalSwitch(); break;
					case 6: BackingSlowSwitch();  break;
					case 7: TouchingSwitch(); break;
					case 8: TeasingSwitch(); break;
					case 9: KnockingSwitch(); break;
					case 10: BuzzingSwitch(); break;
					case 11: RotationSwitch(); break;
					case 12: NormalSwitch(); break;
					case 13: SlowSwitch(); break;
					}
				}
				break;
			}
		}
		/*tryb swiatloluba*/
		if (digitalRead(mode_switchPin) == LOW)
		{
			if (HandCheck() == true)
			{
				Buzz(150);
				timer = millis();
				/*sprawdzenie czy reka zostanie przytrzymana przez 1s*/
				while ((HandCheck() == true) && (millis() - timer < 1000)) {}
				/*jestli tak to albo wlacz albo wylacz*/
				if (millis() - timer >= 1000)
				{
					switch (swiatlolubOnOff)
					{
					case true: swiatlolubOnOff = false; Serial.println("OFF"); Buzz(500); break;
					case false: swiatlolubOnOff = true; Serial.println("ON"); Buzz(500); break;
					}
				}
			}

			if (swiatlolubOnOff == true)
			{
				StandByOff();
				left_track.ChangeDirection(FORWARD);
				right_track.ChangeDirection(FORWARD);

				rightFotoVal = constrain(analogRead(rightFotoPin), 20, 700);
				leftFotoVal = constrain(analogRead(leftFotoPin), 20, 700);

				//Serial.print("Lewy:\t"); Serial.print(leftFotoVal);
				//Serial.print("\tPrawy:\t"); Serial.print(rightFotoVal);
				//Serial.println();

				if (abs(rightFotoVal - leftFotoVal) > deltaFotoTrig)
				{
					if (rightFotoVal > leftFotoVal)
					{
						left_track.ChangeSpeed(255);
						right_track.ChangeSpeed(0);
						left_track.Rotate();
						right_track.Rotate();
						timer2 = millis();
					}
					else
					{
						right_track.ChangeSpeed(255);
						left_track.ChangeSpeed(0);
						right_track.Rotate();
						left_track.Rotate();
						timer2 = millis();

					}
				}
				else
				{
					left_track.ChangeSpeed(normalSpeed); right_track.ChangeSpeed(normalSpeed);
					if (millis() - timer2 > 2000)
					{
						left_track.Stop(); right_track.Stop();
					}
				}
			}
			else
			{
				left_track.Stop(); right_track.Stop();
				StandByOn();
			}
		}
	}
}

void StandByOn()
{
	digitalWrite(stdbPin, LOW);
}

void StandByOff()
{
	digitalWrite(stdbPin, HIGH);
}

void Move(int _time, byte _direction)
{
	if (_direction == 1) //forward
	{
		StandByOff();
		left_track.ChangeDirection(FORWARD);
		right_track.ChangeDirection(FORWARD);
		left_track.Rotate();
		right_track.Rotate();
		delay(_time);
		left_track.Stop();
		right_track.Stop();
		StandByOn();
	}
	if (_direction == 0) //reverse
	{
		StandByOff();
		left_track.ChangeDirection(REVERSE);
		right_track.ChangeDirection(REVERSE);
		left_track.Rotate();
		right_track.Rotate();
		delay(_time);
		left_track.Stop();
		right_track.Stop();
		StandByOn();
	}
}
void Rotation(int _time, byte _direction, byte _speed)
{
	if (_direction == 1) // obrot w prawo (zgodnie ze wskazowkami zegara
	{
		StandByOff();
		left_track.ChangeDirection(FORWARD); left_track.ChangeSpeed(_speed);
		right_track.ChangeDirection(REVERSE); right_track.ChangeSpeed(_speed);
		left_track.Rotate(); right_track.Rotate();
		delay(_time);
		left_track.Stop(); right_track.Stop();
		left_track.ChangeSpeed(255); right_track.ChangeSpeed(255);
		StandByOn();

	}
	if (_direction == 0) //obrot w lewo (przeciwnie do wskazowek zegara)
	{
		StandByOff();
		left_track.ChangeDirection(REVERSE); left_track.ChangeSpeed(_speed);
		right_track.ChangeDirection(FORWARD); right_track.ChangeSpeed(_speed);
		left_track.Rotate(); right_track.Rotate();
		delay(_time);
		left_track.Stop(); right_track.Stop();
		left_track.ChangeSpeed(255); right_track.ChangeSpeed(255);
		StandByOn();
	}
}
void Buzz(int _time)
{
	digitalWrite(buzzPin, HIGH);
	delay(_time);
	digitalWrite(buzzPin, LOW);
}

bool HandCheck() {
	long czas, dystans;

	digitalWrite(trigPin, LOW);
	delayMicroseconds(2);
	digitalWrite(trigPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(trigPin, LOW);

	czas = pulseIn(echoPin, HIGH);
	dystans = czas / 58;

	if (dystans < HandDist) return true;
	else return false;
}

int HandDistance()
{
	long czas, dystans;
	digitalWrite(trigPin, LOW);
	delayMicroseconds(2);
	digitalWrite(trigPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(trigPin, LOW);

	czas = pulseIn(echoPin, HIGH);
	dystans = czas / 58;
	return (int)dystans;
}
void NormalSwitch()
{
	delay(100);
	hand_servo.write(max_servo_posit);
	delay(500);
	hand_servo.write(basic_servo_posit);
	delay(250);
}

void SlowSwitch()
{
	int pozycja;
	for (pozycja = basic_servo_posit; pozycja <= max_servo_posit; pozycja++)
	{
		hand_servo.write(pozycja);
		delay(8);
	}
	for (pozycja = max_servo_posit; pozycja >= basic_servo_posit; pozycja--)
	{
		hand_servo.write(pozycja);
		delay(8);
	}
}

void UltraSlowSwitch()
{
	int pozycja;
	//hand_servo.write(basic_servo_posit + 1);
	for (pozycja = basic_servo_posit; pozycja <= max_servo_posit; pozycja++)
	{
		hand_servo.write(pozycja);
		delay(40);
	}
	for (pozycja = max_servo_posit; pozycja >= basic_servo_posit; pozycja--)
	{
		hand_servo.write(pozycja);
		delay(40);
	}
}

void SlowAppear()
{
	int pozycja;
	for (pozycja = basic_servo_posit; pozycja <= max_servo_posit; pozycja++)
	{
		hand_servo.write(pozycja);
		delay(20);
	}
	hand_servo.write(basic_servo_posit);
	delay(500);
}

void SlowDisappear()
{
	int pozycja;
	delay(100);
	hand_servo.write(max_servo_posit);
	delay(500);
	for (pozycja = max_servo_posit; pozycja >= basic_servo_posit; pozycja--)
	{
		hand_servo.write(pozycja);
		delay(20);
	}

}

void HandRunBack()
{
	if (HandCheck() == true)
	{
		Buzz(150);
		Move(500, RVRS);
		backingCount++;
	}
}

void BackingNormalSwitch()
{
	Move(750, RVRS);
	NormalSwitch();
	delay(300);
	Move(750, FRWD);
}

void BackingSlowSwitch()
{
	Move(750, RVRS);
	UltraSlowSwitch();
	Move(750, FRWD);
}

void TouchingSwitch()
{
	delay(100);
	hand_servo.write(touch_servo_posit);
	delay(3000);
	hand_servo.write(max_servo_posit);
	Buzz(200);
	hand_servo.write(basic_servo_posit);
	delay(500);
}

void TeasingSwitch()
{
	hand_servo.write(door_servo_posit);
	delay(200);
	int pozycja;
	for (pozycja = door_servo_posit; pozycja <= 60; pozycja++)
	{
		hand_servo.write(pozycja);
		delay(10);
	}
	hand_servo.write(door_servo_posit);
	delay(1000);
	for (pozycja = door_servo_posit; pozycja <= 90; pozycja++)
	{
		hand_servo.write(pozycja);
		delay(10);
	}
	hand_servo.write(door_servo_posit);
	delay(1500);
	for (pozycja = door_servo_posit; pozycja <= touch_servo_posit; pozycja++)
	{
		hand_servo.write(pozycja);
		delay(10);
	}
	delay(100);
	hand_servo.write(door_servo_posit);
	delay(3000);
	hand_servo.write(max_servo_posit);
	delay(500);
	hand_servo.write(basic_servo_posit);
	delay(500);
}

void KnockingSwitch()
{
	int i;
	for (i = 0; i < 5; i++)
	{
		hand_servo.write(door_servo_posit + 10);
		delay(200);
		hand_servo.write(door_servo_posit - 10);
		delay(200);
	}
	hand_servo.write(max_servo_posit);
	delay(500);
	hand_servo.write(door_servo_posit - 10);
	delay(1000);
	for (i = 0; i < 2; i++)
	{
		hand_servo.write(door_servo_posit + 10);
		delay(200);
		hand_servo.write(door_servo_posit - 10);
		delay(200);
	}
	hand_servo.write(basic_servo_posit);
	delay(500);
}

void BuzzingSwitch()
{
	int i;
	for (i = 1050; i >= 50; i -= 100)
	{
		Buzz(i);
		delay(constrain(i - 500, 50, 1500));
	}
	for (i = 50; i >= 10; i -= 5)
	{
		Buzz(i);
		delay(i);
	}
	NormalSwitch();
	delay(500);
}

void RotationSwitch()
{
	Rotation(rotationTime / 2, RIGHT, 255);
	NormalSwitch();
	delay(300);
	Rotation(rotationTime / 2, RIGHT, 255);
}
