//includes servo library so servo can be controlled
#include <Servo.h>
//function declarations
int findNextCol();
void rotateFlag(int currPos);
void correctMoveIndicator();
void incorrectMoveIndicator();
void celebrate();
void despair();
void resetBoard(int newGame);
void printPhotoCellStates();
void createPath(int patternPtr[4][3], int numRows, int numCols);
int generateHorizontal(int patternPtr[4][3], int startingRow, int startingCol, int numCols);
int generateVertical(int patternPtr[4][3], int startingRow, int startingCol, int numCols);
void printPath(int patternPtr[4][3], int numRows, int numCols);

//stores the pin values of each photoresistor
int photoCells[6] = {A0, A1, A2, A3, A4, A5};
//stores the state of each photoresistor
int photoCellStates[6] = {0, 0, 0, 0, 0, 0};
//stores the values outputted to the serial monitor by each photoresistor when uncovered before the game starts
//helps with determining the threshold for which the photoresistors are considered to be turned off
int ambientLightValues[6] = {0, 0, 0, 0, 0, 0};

//stores pin value for button used to start game
int onButtonPin = 2;
int onButtonState = 0;

//stores pin value for button used to replay game with same path
int replayButtonPin = 3;
int replayButtonState = 0;

//stores pin values for LEDs that represent number of lives player has left
int lifeLEDPins[3] = {4, 5, 6};
//Stores pin values for LEDs used to indicate the column of the position the player last chose. Helps them remember their last choice better.
int colLEDPins[3] = {7, 8, 9};
//stores pin value for buzzer
int buzzerPin = 10;

//creates servo object used to rotate flag
Servo flagServo;
Servo clockServo;

//array that holds the values for the correct path
int path[4][3] = {{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}};
//used to store number of lives player has, initially set to 3
int numLives = 3;

//tracks which step in the path sequence the user is currently on
int stepTracker = 1;
//Records total number of steps in the path. Set to 1 by default before path generation
int totalSteps = 1;
//records the row the user made their previous move in
int currRow = 0;
//records the column corresponding to the spot where the user made their previous move
int prevCol = 0;
//used to indicate whether the user wants to play game again with same path
int playAgain = 0;

//holds the number of degrees the servo needs to turn by for each correct move
//initially set to 0 but given a real value later 
int nextFlagRotation = 0;
//holds current position of the flag
int flagPos = 0;

//records total time passed since start of program
unsigned long timePassed = 0;
//Stores time passed at a given point. Used to help determine how long certain loops should run.
unsigned long loopTimeStart = 0;

void setup()
{
  for(int i = 0; i < 5; i++)
  {
    //sets all photoresistor pins to input mode
    pinMode(photoCells[i], INPUT);
    //only does this 3 times in this loop so a second loop that runs only 3 times isn't needed
    if(i < 3)
    {
      //sets pins for lifeLEDs to output mode
      pinMode(lifeLEDPins[i],OUTPUT);
      //turns on each lifeLED
      digitalWrite(lifeLEDPins[i], HIGH);
      //sets pins for colLEDs to output mode
      pinMode(colLEDPins[i], OUTPUT);
    }
  }
  //sets pin for onButton to input mode
  pinMode(onButtonPin, INPUT);
  //sets pin for replayButton to input mode
  pinMode(replayButtonPin, INPUT);
  //attaches flagServo to pin 11 so it can be controlled
  flagServo.attach(11);
  //begins communication with serial monitor
  Serial.begin(9600);
}

void loop()
{
  //constantly checks if onButton is pressed
  onButtonState = digitalRead(onButtonPin);
  //continuously updates timePassed
  timePassed = millis();
  //begins game if pressed
  if (onButtonState == HIGH) 
  {
    //print statements for debugging to make sure button was pressed
    Serial.println(onButtonState);
    Serial.println("Pressed");
    //short delay to give photoresistors time to adjust
    delay(1000);
    //Creates a seed for random number generation. Uses time button was pressed at since all analog pins are taken.
    randomSeed(millis());
    //rotates flag to 0 degrees
    flagServo.write(0);
    //sets to 1 each time so if game has been played before total steps will have the correct value
    totalSteps = 1;
    //creates the randomly generated path
    createPath(path, 4, 3);
    //decreases total number of steps for clarity purposes since it's technically 1 larger than the actual number of steps
    totalSteps--;
    //calculates how many degrees flag needs to rotate for each correct move
    nextFlagRotation = 90 / (totalSteps);
    //prints path for debugging purposes
    printPath(path, 4, 3);

    //reads and prints out each input value of each photocell for debugging purposes
    photoCellStates[0] = analogRead(photoCells[0]);
  	photoCellStates[1] = analogRead(photoCells[1]);
  	photoCellStates[2] = analogRead(photoCells[2]);
  	photoCellStates[3] = analogRead(photoCells[3]);
  	photoCellStates[4] = analogRead(photoCells[4]);
  	photoCellStates[5] = analogRead(photoCells[5]);
    Serial.print("A0: ");
  	Serial.println(photoCellStates[0]);
  	Serial.print("A1: ");
  	Serial.println(photoCellStates[1]);
  	Serial.print("A2: ");
  	Serial.println(photoCellStates[2]);
  	Serial.print("A3: ");
  	Serial.println(photoCellStates[3]);
  	Serial.print("A4: ");
  	Serial.println(photoCellStates[4]);
  	Serial.print("A5: ");
  	Serial.println(photoCellStates[5]);
    Serial.println("Start now!");

    //do-while loop where the main actions of the game occur. Keeps repeating as long as replay button is pressed at the end of each game.
    do 
    {
      //records current time passed to help calculate how long the loop needs to run for
      loopTimeStart = millis();
      
      //reads input value of each photocell and stores it in ambientLightValues 
      for(int i = 0; i < 6; i++)
      {
        photoCellStates[i] = analogRead(photoCells[i]);
        //Sometimes photocell values are too low. Values were tested to be around 310 at site where robot will be located, so
        //it changes values that are randomly too low to 310 since that's the value they should be at.
        if(photoCellStates[i] < 310)
        {
          photoCellStates[i] = 310;
        }
        ambientLightValues[i] = photoCellStates[i];
      }

      //Loop where one playthrough of the game occurs. Runs until last step has been reached, all lives lost, or time runs out.
      while (stepTracker <= totalSteps && numLives > 0 && timePassed < loopTimeStart + 60000) 
      {
        //Updates time passed since start of program. Helps determine when time is up and loop should end.
        timePassed = millis();
        //reads each photoresistor value to see if any are covered
        photoCellStates[0] = analogRead(photoCells[0]);
  		  photoCellStates[1] = analogRead(photoCells[1]);
  		  photoCellStates[2] = analogRead(photoCells[2]);
  		  photoCellStates[3] = analogRead(photoCells[3]);
  		  photoCellStates[4] = analogRead(photoCells[4]);
  		  photoCellStates[5] = analogRead(photoCells[5]);
		
        //checks each of the 6 photoresistors to see if one was turned off
        for(int i = 0; i < 6; i++)
        {
          //indicates whether the player chose the current row or the next row
          int chosenRow = 0;
          //Records column chosen by the player. Set to 2-i since generated path is facing opposite direction on board, and needs to be rotated around to function properly.
          int chosenCol = 2-i;
          //correctly updates chosenRow and chosenCol if the photoresistors in the next row are chosen instead of the current row
          if(i > 2)
          {
            chosenRow = 1;
            chosenCol = 5-i;
          }
          //checks to see if the incorrect photoresistor was pressed
          if (photoCellStates[i] < ambientLightValues[i]*0.45
              && (currRow + chosenRow > 4 || path[currRow+chosenRow][chosenCol] != stepTracker)) 
          {
            //prints out photocell values for debugging
            printPhotoCellStates();
            //player loses a life
            numLives--;
            //one lifeLED turns off to represent life lost
            digitalWrite(lifeLEDPins[numLives], LOW);
            //sound indicating move was incorrect
            incorrectMoveIndicator();
            //short delay to prevent program from registering photoresistor being pressed multiple times
            delay(1200);
            //print statement for debugging
            Serial.println("Incorrect!");
       	  } 
          //checks to see if the correct photoresistor was pressed
          else if (photoCellStates[i] < ambientLightValues[i]*0.45 && path[currRow+chosenRow][chosenCol] == stepTracker) 
          {
            //prints out photocell values for debugging
            printPhotoCellStates();
            //increases stepTracker by 1 to record how far the user has gotten in the maze
            stepTracker++;
            //increases current row by 1 if the chosen row was the next row
            currRow += chosenRow;
            //turns off the LED indicating the column of the previous move since a new move has been made
            digitalWrite(colLEDPins[prevCol], LOW);
            //turns on the LED indicating the column of the move just made to help user know where their last move was
            digitalWrite(colLEDPins[2-chosenCol], HIGH);
            //updates prevCol to same column as the column of the move just made
            prevCol = 2-chosenCol;
            //print statement for debugging
            Serial.println("Correct!");
            //rotates flag by degrees corresponding to number of steps in path
            rotateFlag();
            //sound used to indicate the correct move
            correctMoveIndicator();
            //print statements for debugging
            Serial.print("StepTracker: ");
            Serial.println(stepTracker);
            Serial.print("TotalSteps: ");
            Serial.println(totalSteps);
            //short delay to prevent program from registering photoresistor as being pressed multiple times
            delay(1200);
          }
        }
      }
      //checks if player made it to end of path without losing all their lives, which means they won
      if (numLives > 0 && stepTracker > totalSteps) 
      {
        //print statment for debugging
        Serial.println("You won!");
        //celebratory noises and LEDs flash
        celebrate();
        //makes player unable to play game with same path again
        playAgain = 0;
      } 
      //happens if player loses all their lives or if time runs out
      else 
      {
        //print statement for debugging
        Serial.println("You lost!");
        //sad noises and flashing lifeLEDs to indicate a loss
        despair();
        //print statement for debugging
        Serial.println("Press the button if you want to play again");
        //updates timePassed and loopTimeStart to help calculate how long next loop should run for
        timePassed = millis();
        loopTimeStart = millis();
        //gives player 15 seconds to decide if they want to play the game again with the same path
        while (timePassed < loopTimeStart + 15000) 
        {
          timePassed = millis();
          replayButtonState = digitalRead(replayButtonPin);
          //if replay button pressed, player gets to play game again using same path
          if (replayButtonState == HIGH) 
          {
            //sets playAgain to 1 to indicate player wants to play game again with same path
            playAgain = 1;
            //print statement for debugging
            Serial.println("You are playing the game again");
            //short delay to prevent program from registering button being pressed multiple times
            delay(250);
            loopTimeStart = millis();
            //Resets elements in circuit to prepare to play again. 0 passed in to indicate that it's only being reset for replay purposes, not an entirely new game
            resetBoard(0);
            //breaks out of loop
            break;
          }
          playAgain = 0;
        }
      }
    } while (playAgain); //repeats whole process again if player chooses to play game again, and stops if user doesn't choose to play again
    //completely resets elements in circuit to prepare board so new game can begin later
    resetBoard(1);
    //print statement for debugging
    Serial.println("You are done playing the game");
  }
}

//rotates flag appropriate number of degrees
void rotateFlag() 
{
  //rotates flag
  for (int pos = flagPos; pos < flagPos + nextFlagRotation; pos++) 
  {
    flagServo.write(pos);
    delay(10);
  }
  //updates flag position to current position of flag
  flagPos += nextFlagRotation;
}

//function that indicates correct move with a high-pitch sound
void correctMoveIndicator() 
{
  tone(buzzerPin, 400);
  delay(750);
  noTone(buzzerPin);
}

//function that indicates correct move with a lower-pitch sound
void incorrectMoveIndicator() 
{
  tone(buzzerPin, 200);
  delay(750);
  noTone(buzzerPin);
}

//function that prints the value outputted by each photoresistor for debugging purposes
void printPhotoCellStates()
{
  for(int i = 0; i < 6; i++)
  {
    Serial.print("A");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(photoCellStates[i]);
    Serial.print(" and ");
    Serial.println(ambientLightValues[i]);
  }
}

//function used to indicate user has won the game with flashing lights and happy sounds
void celebrate()
{
  //turns LEDs off before celebrating
  for(int i = 0; i < 3; i++)
  {
    digitalWrite(lifeLEDPins[i], LOW);
    digitalWrite(colLEDPins[i], LOW);
  }
  
  //flashes all LEDs and makes buzzer buzz 8 times at rate of 2 Hz
  for(int i = 0; i < 8; i++)
  {
    digitalWrite(lifeLEDPins[0], HIGH);
    digitalWrite(colLEDPins[0], HIGH);
    digitalWrite(lifeLEDPins[1], HIGH);
    digitalWrite(colLEDPins[1], HIGH);
    digitalWrite(lifeLEDPins[2], HIGH);
    digitalWrite(colLEDPins[2], HIGH);
    tone(buzzerPin, 400);
    delay(250);
    digitalWrite(lifeLEDPins[0], LOW);
    digitalWrite(colLEDPins[0], LOW);
    digitalWrite(lifeLEDPins[1], LOW);
    digitalWrite(colLEDPins[1], LOW);
    digitalWrite(lifeLEDPins[2], LOW);
    digitalWrite(colLEDPins[2], LOW);
    noTone(buzzerPin);
    delay(250);
  }
}

//function used to indicate player has lost the game with flashing lifeLEDs and sad sounds
void despair()
{
  //turns off LEDs before despairing
  for(int i = 0; i < 3; i++)
  {
    digitalWrite(lifeLEDPins[i], LOW);
    digitalWrite(colLEDPins[i], LOW);
  }
  
  //flashes lifeLEDs and makes buzzer buzz 6 times at rate of 2 Hz
  for(int i = 0; i < 6; i++)
  {
    digitalWrite(lifeLEDPins[0], HIGH);
    digitalWrite(lifeLEDPins[1], HIGH);
    digitalWrite(lifeLEDPins[2], HIGH);
    tone(buzzerPin, 200);
    delay(250);
    digitalWrite(lifeLEDPins[0], LOW);
    digitalWrite(lifeLEDPins[1], LOW);
    digitalWrite(lifeLEDPins[2], LOW);
    noTone(buzzerPin);
    delay(250);
  }
}

//resets LEDs and several variables to set up board and code so it can be replayed
//int passed in indicates whether board needs to be reset to be replayed with same path or completely reset and use a newly generated path
void resetBoard(int newGame)
{
  stepTracker = 1;
  //resets number of lives
  numLives = 3;
  //currRow and prevCol set back to 0 so last position isn't recorded for new round
  currRow = 0;
  prevCol = 0;
  //turns flag back to original position
  flagServo.write(0);
  flagPos = 0;
  //turns on the lifeLEDs
  for (int i = 0; i < 3; i++) 
  {
    digitalWrite(lifeLEDPins[i], HIGH);
  }
  
  //runs if a completely new game is occuring instead of just replaying the game with same path
  if(newGame)
  {
    //sets every value in the path array to 0 so it's reset properly
	  for(int r = 0; r < 4; r++)
    {
      for(int c = 0; c < 3; c++)
      {
        path[r][c] = 0;
      }
    }
    //sts totalSteps back to 1 since that's what it normally is supposed to be at with no path generated
    totalSteps = 1;
  }
}

//Creates a randomly generated path tile by tile. Eahc path tile generates either horizontally, vertically, or diagonally. Never goes backwards.
void createPath(int patternPtr[4][3], int numRows, int numCols)
{
  int r = 0;
  //randomly chooses a column to start at
  int c = random(numCols);
  //sets this column equal to totalSteps
  //totalSteps is updated so that the correct sequence is recorded within the path. Each number indicates which step in the path the tile is located in.
  //For example, the first tile will generate in the array with the value 1, to indicate it's the first step in the path. The next tile will generate with the value 2 to 
  //indicate it's the second step in the path, etc.
  patternPtr[0][c] = totalSteps++;
  //keeps generating steps in path until it reaches the final row of the array, which is always the last step in the path
  while(r < numRows-1)
  {
    //if 0, then generate a step either directly forward or diagonal, and if 1, generate step horizontally
     int horizontalOrVertical = random(2);
     //generates a step horizontally (in the same row as the previous step) if randomly chosen and if the path hasn't been backed into a corner
     //if path is "backed into a corner," that means either the row is completley full or heads in the direction of and reaches the first or final column
     //an example of the latter would be:
     //0 1 2
     //so if 2 is the previous step generated, no more tiles can generate horizontally in that row, as the path must move forward at this point
     if(horizontalOrVertical && !((c == numCols - 1 && patternPtr[r][c-1]) || (c == 0 && patternPtr[r][c+1])))
     {
      //sets c equal to the column number of the chosen location
       c = generateHorizontal(patternPtr, r, c, numCols);
     }
     //generates a step in the path vertically otherwise
     else
     {
       //sets c equal to the column number of the chosen location
       c = generateVertical(patternPtr, r, c, numCols);
       //increases r by one since path moved forward by 1 row
       r++;
     }
  }
}

//function used to generate a step of the path in the horizontal direction
int generateHorizontal(int patternPtr[4][3], int startingRow, int startingCol, int numCols)
{ 
  int newCol;
  //checks if the previous step is located in the last column of the path or if the tile to the right is already filled 
  if(startingCol == numCols - 1 || patternPtr[startingRow][startingCol+1])
  {
    //generates step to the left of the current step
    patternPtr[startingRow][startingCol - 1] = totalSteps++;
    //updates newCol to col value of new step
    newCol = startingCol - 1;
  }
  //checks if the previous step is located in the first colum of the path or if the tile to the left is already filled
  else if(startingCol == 0 || patternPtr[startingRow][startingCol-1])
  {
    //generates step to the right of the current step
    patternPtr[startingRow][startingCol+1] = totalSteps++;
    //updates newCol to col value of new step
    newCol = startingCol + 1;
  }
  //occurs only if previous step is in the middle column
  else
  {
    //0 means generate step left, 1 means generate step right
    int leftOrRight = random(2);
    //checks if right was chosen
    if(leftOrRight)
    {
      //generates step to the right of the current tile
      patternPtr[startingRow][startingCol+1] = totalSteps++;
      //updates newCol to col value of new step
      newCol = startingCol + 1;
    }
    //runs if left was chosen
    else
    {
      //generates step to the left of the current tile
      patternPtr[startingRow][startingCol-1] = totalSteps++;
      //updates newCol to col value of new step
      newCol = startingCol - 1;
    }
  }
  return newCol;
}

//function used to generate a step either directly forward or diagonally
int generateVertical(int patternPtr[4][3], int startingRow, int startingCol, int numCols)
{
  //variable used to hold the direction the path generates in
  int direction = 0;
  //checks if column of previous move is int the last column
  if(startingCol == numCols-1)
  {
    //changes value of direction so that it randomly goes either left diagonal (equal to -1) or directly forward (equal to 0)
    direction = random(2);
    direction--;
  }
  //checks if column of previous moe is in the first column
  else if(startingCol == 0)
  {
    //changes value of direction so that it randomly goes either right diagonal (equal to 1) or directly forward (equal to 0) 
    direction = random(2);
  }
  //previous move was in the middle column
  else
  {
    //changes value of direction so that it randomly goes either left diagonal (-1) directly forward (0) or right (1)
    direction = random(3);
    direction--;
  }

  //sets next value in the path chosen based on direction chosen above equal to totalSteps
  patternPtr[startingRow+1][startingCol+direction] = totalSteps++;
  return startingCol+direction;
}

//function that prints out every value in the path array for debugging purposes
void printPath(int patternPtr[4][3], int numRows, int numCols)
{
  int r;
  int c;
  //prints out every value in the path
  for(r = 0; r < numRows; r++)
  {
    for(c = 0; c < numCols; c++)
    {
      Serial.print(patternPtr[r][c]);
      Serial.print(" ");
    }
    Serial.println();
  }
}