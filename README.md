Sure, here is a detailed README file for your Arduino Slot Machine project:

---

**Name: Theodor-Ioan Rolea**

**Group: 333CA**

# Arduino Slot Machine

## Overview
This project implements a Slot Machine using an Arduino microcontroller. The slot machine features multiple hardware components, including LED matrices, a buzzer, an LCD screen, and segment displays. The goal of the project is to create a fully functional slot machine that can simulate the experience of playing a real slot machine, complete with spinning wheels, payouts, and sound effects.

***

# Code Structure
The code is organized into several key sections to manage the different functionalities of the slot machine:

1. **Definitions and Inclusions**:
    - Use of `#define DEBUG` to toggle debug mode.
    - Inclusion of essential libraries like `LedControl`, `TimerFreeTone`, `Wire`, `LiquidCrystal_I2C`, and `TM1637Display`.

2. **Pin and Variable Setup**:
    - Definition of pins for buttons, LED matrix, segment display, and buzzer.
    - Initialization of global constants and variables for game control, such as delay times, payout values, and credit balance.

3. **Global Variables and Structures**:
    - Creation of the `spinDigit` structure to store wheel information.
    - Initialization of variables for tracking game statistics and settings.

4. **`setup()` Function**:
    - Configuration of pins and hardware components.
    - Setting up randomization seed and initial display settings.
    - Display of the startup screen and initial wheel symbols.

5. **`loop()` Function**:
    - Waiting for button press to start the game.
    - Spinning the wheels and calculating potential winnings.
    - Updating the credit balance and displaying the current wager.

6. **Utility Functions**:
    - `spinTheWheels()`: Manages the spinning of the slot machine wheels.
    - `displayWheelSymbol()`: Updates the symbols displayed on the wheels.
    - `highlightWinAndCalculatePayout()`: Checks for winning combinations and calculates payouts.
    - `flashSymbol()`: Flashes winning symbols for visual effect.
    - `playSplashScreen()`, `playMelody()`, `winSound()`: Provides sound effects and visual feedback.
    - `adjustCreditBalance()`, `displayWager()`: Manages and displays the player's credit balance.
    - `waitOnButtonPress()`, `waitOnButtonPressDouble()`: Handles button presses for game control and doubling bets.

***

# Development Insights
During the development of this project, I encountered several challenges and gained valuable insights:

- **Hardware Integration**: Interfacing multiple hardware components (LED matrices, LCD, buzzer) required careful management of pin configurations and timing.
- **Timing and Delays**: Fine-tuning the delays for spinning wheels and updating displays was crucial for creating a smooth user experience.
- **Randomization**: Ensuring true randomness in the wheel spins involved using the `randomSeed()` function with analog input for variability.
- **Debugging**: Implementing a debug mode using `#define DEBUG` allowed me to test and troubleshoot the code more effectively.

***

# Results
The final implementation successfully simulates a slot machine experience:

- Wheels spin and display random symbols.
- Winning combinations are highlighted, and payouts are calculated correctly.
- Sound effects enhance the user experience.
- Credit balance and wager information are accurately managed and displayed.

***

# Conclusion
The Arduino Slot Machine project demonstrates the capability of the Arduino platform to manage complex interactions between multiple hardware components. By using structured code and modular functions, the project achieves a functional and engaging slot machine simulation.

# Final Thoughts
This project not only deepened my understanding of Arduino programming and hardware integration but also highlighted the importance of code organization and modularity. Future improvements could include more sophisticated animations, additional sound effects, and enhanced user interface elements.

# References
- Arduino Official Documentation: [Arduino Reference](https://www.arduino.cc/reference/en/)
- LedControl Library: [LedControl Library](https://wayoda.github.io/LedControl/)
- TimerFreeTone Library: [TimerFreeTone Library](https://bitbucket.org/teckel12/arduino-timer-free-tone/wiki/Home)
- LiquidCrystal_I2C Library: [LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C)
- TM1637Display Library: [TM1637Display](https://github.com/avishorp/TM1637)

---
