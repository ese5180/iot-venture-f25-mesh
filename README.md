[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/9GQ6o4cu)
# IoT Venture Pitch
## ESE5180: IoT Wireless, Security, & Scaling

**Team Name: Mesh** 

| Team Member Name | Email Address       |
|------------------|---------------------|
| Zeng Li          |lizeng@seas.upenn.edu|
| Haichao Zhao         |haichao@seas.upenn.edu           |
| [Name 3]         | [Email 3]           |
| [Name 4]         | [Email 4]           |

**GitHub Repository URL:https://github.com/Lizzzzzz1122/Desk_Car.git**

## Concept Development

### Product Function

The smart mouthguard is a wireless-enabled nighttime dental appliance, which integrates multiple sensors to monitor bruxism, oral condition and sleep position changes.

### Target Market & Demographics

### Stakeholders

### System-Level Diagrams

#### **Device Block Diagram**
<div align="center">
<img src="images/Power_Block_Dia.png" alt="device block diagram">
</div>

#### **Communication Diagram**
<div align="center">
<img src="images/Pro_Block_Dia.png" alt="device block diagram">
</div>

### Security Requirements Specification

#### **Overview**

This section describes the security systems required for the smart dental brace project, which collects sensitive biometric data including bite force, oral temperature/humidity, and head posture through embedded sensors. The system transmits this health data via Bluetooth LE to external devices for analysis and storage.

#### **Definitions, Abbreviations**

**PHI** - Protected Health Information  

**BLE** - Bluetooth Low Energy  

**AES** - Advanced Encryption Standard 

**OTP** - One-Time Programmable memory

**IEC 62304** - International standard for medical device software lifecycle processes

**FDA** - Food and Drug Administration (US medical device regulatory body)

#### **Functionality**

**SEC 01 –** Biometric data (bite force, temperature, humidity, IMU readings) shall be encrypted using AES-128 or higher before wireless transmission

**SEC 02 –** All cryptographic keys and device credentials shall be stored in the nRF5340's secure OTP memory region with hardware-level protection

**SEC 03 –** The BLE connection shall implement authentication and authorization before allowing access to sensitive health data

**SEC 04 –** The device shall implement secure boot functionality to prevent unauthorized firmware modifications

**SEC 05 –** The system shall comply with medical device security standards (IEC 62304, FDA cybersecurity guidelines)


### Hardware Requirements Specification

Nordic 

### Software Requirements Specification


