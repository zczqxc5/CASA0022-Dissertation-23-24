# TinyML-Based Wearable Device for Smart Home Control
![c65d1a3ddbf9a18497049976181c432](https://github.com/user-attachments/assets/c0966193-d518-424d-9de7-6bd27cde3b9c)

## How to use device
Uploading copy_A15FBF07-489B-4D9C-815F-8CE176EFDEB2.mp4…


## Project Overview

This project integrates Tiny Machine Learning (TinyML) technology into low-cost, microcontroller-based wearable devices to facilitate seamless and precise smart home operations for individuals with hearing and speech challenges. By recognizing American Sign Language (ASL) gestures, the system allows users to control smart home devices without the need for voice commands.
### Key Features:
- Gesture recognition for controlling smart home devices
- Vibration alerts for environmental sounds (e.g., door knocking)
- Real-time display of environmental data (e.g., temperature, humidity) using gestures
  
### Research Question
To what extent can the integration of TinyML technology in low-cost microcontroller-based wearable devices facilitate seamless and precise smart home operations for individuals with hearing and speech challenges?

## Table of Contents

- [Project Overview](#project-overview)
- [Hardware Used](#hardware-used)
- [Software Used](#software-used)
- [System Architecture](#system-architecture)
- [Data Collection](#data-collection)
- [Model Development](#model-development)
- [Results](#results)
- [Limitations](#limitations)
- [Future Work](#future-work)


## Hardware Used

- **Microcontroller**: Seeed Studio XIAO BLE Sense
  - ARM Cortex-M4 Processor
  - 9-axis IMU (Accelerometer, Gyroscope, Magnetometer)
  - Bluetooth 5.0 (BLE)
- **Smart Home Components**:
  - ESP32 Microcontroller (for smart home model)
  - LED lights, Servo motor, Buzzer, DHT22 Sensor (for smart home controls)
- **Wearable Device Components**:
  - Vibrating motor for feedback
  - 1.69-inch LCD IPS screen for environmental data display
  - Li-Po Battery 3.7V 140mAh
 
  
![image](https://github.com/user-attachments/assets/129ab376-f153-49dd-b774-f51ccecc1fc9)

![4c1fe1656bfd53e33929082f7ee5c43](https://github.com/user-attachments/assets/80368752-2ae6-4918-bda7-3913d7156ce5)


## Software Used

- **Arduino IDE**: For writing and uploading code to microcontrollers.
- **Edge Impulse**: Platform used for TinyML model development. The data and model can be copied from: https://studio.edgeimpulse.com/public/405924/live
- **TensorFlow Lite**: Used to deploy the trained model on the microcontroller.
- **Fusion 360**: Used for PCB and enclosure design.

## System Architecture

The system is designed to collect gesture data from a wearable device, process it using TinyML models, and send the results to control smart home devices. The smart home devices, including lights, curtains, and speakers, are controlled based on the recognized ASL gestures.
![Untitled (1)](https://github.com/user-attachments/assets/aa916571-a28e-4bd8-a52e-ebfd9ff11232)

## How to reproduced

1. Prepare hardwares and softwares from above list
2. Making PCB then conneted hardwares or connected them as circuits
3. Use Egen Impluse platform to build deployment of "Arduino Library", and import it into Arduino IDE
4. Upload code to Xiao BLE Sense/Esp32 respectively from file [Code-XiaoBLESense] /[Code-ESP32]
5. 3d print(PLA material) and laser cut(3mm wood) the enclousure from file [Enclousure]
   

## Data Collection

IMU data (accelerometer and gyroscope) was collected using the Seeed Studio XIAO BLE Sense to recognize 7 different ASL gestures:
- "Hello"
- "Bye"
- "Curtain"
- "Light"
- "Music"
- "Display"
- "Other"

A total of 2,149 samples were collected from 20 participants over 7 gesture categories. The data was split into 80% for training and 20% for testing.
![poster](https://github.com/user-attachments/assets/99984ab2-7c10-43bf-9b5b-87e06d96af4b)

## Model Development

The TinyML model was developed using Edge Impulse and deployed on the Seeed Studio XIAO BLE Sense. The model uses the following techniques:
- **Spectral Analysis**: To extract frequency and power characteristics from IMU data.
- **Low-Pass Filter**: Applied to remove high-frequency noise.
- **Neural Network Architecture**: Designed to optimize gesture recognition with an accuracy of 98.79% during testing.

## Results

### Model Performance:
- **Testing Accuracy**: 98.79%
- **User Testing Accuracy**: 91.28%

The model demonstrated high accuracy in recognizing gestures like "Curtain," "Music," and "Bye," with over 95% accuracy. However, some gestures, such as "Light" and "Display," experienced slight confusion, which impacted real-world user testing.

### User Testing:
7 participants tested the system in a simulated smart home environment, and the overall accuracy during these tests was 91.28%. The average response time for smart home operations was 1.2 seconds.

### Confusion Matrix:
|              | Predicted Curtain | Predicted Light | Predicted Music | Predicted Display | Predicted Hello | Predicted Bye | Predicted Other |
|--------------|-------------------|----------------|-----------------|------------------|----------------|---------------|-----------------|
| Actual Curtain | 60                | 0              | 0               | 0                | 0              | 0             | 3               |
| Actual Light   | 0                 | 57             | 0               | 4                | 0              | 0             | 2               |
| Actual Music   | 0                 | 0              | 60              | 0                | 0              | 0             | 3               |
| Actual Display | 0                 | 3              | 0               | 59               | 0              | 0             | 1               |
| Actual Hello   | 0                 | 1              | 0               | 0                | 59             | 2             | 1               |
| Actual Bye     | 0                 | 0              | 0               | 0                | 1              | 61            | 1               |

## Limitations

- **Gesture Confusion**: Similar gestures like "Light" and "Display" were sometimes confused, impacting real-world usability.
- **User Testing**: While the model performed well in controlled environments, real-world accuracy dropped to 91.28%, indicating room for improvement in diverse conditions.

## Future Work

- **Improve Gesture Recognition**: Focus on refining the recognition of similar gestures like "Light" and "Display."
- **Expand System Capabilities**: Integrate more smart home devices and explore more advanced user interfaces (e.g., haptic feedback).
- **Larger-Scale Testing**: Conduct more extensive user testing with diverse participants to improve model generalizability.


## Acknowledgements

This project was developed as part of my dissertation for the Connected Environments module at University College London. Special thanks to my advisor, Andy Hudson-Smith and all of Connected Environment tutors, for their guidance and support.

