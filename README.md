# BioHaptics — Interactive Soft-Tissue Simulator

BioHaptics is an ongoing C++ engineering project focused on interactive soft-tissue deformation, virtual instrument contact and simulated haptic force modelling.

The long-term goal is to develop a custom physical haptic interface that allows users to interact with virtual tissue and experience controlled force feedback.

## Current Features

- Interactive 3D virtual probe controlled using mouse input
- Deformable tissue mesh with Gaussian-based indentation
- Smooth tissue recovery after contact
- Probe–tissue contact detection
- Spring-damper model for simulated contact forces
- Force limiting and signal smoothing
- Real-time 3D visualisation

## Technologies Used

- C++
- CHAI3D 3.3.0
- OpenGL
- GLFW
- Visual Studio 2022
- CMake

## Hardware Development

Physical hardware integration is currently being planned.

Components include:
- ESP32 microcontroller
- B10K linear slide potentiometer
- MGN12 linear guide rail with carriage
- M3 mounting hardware
- 3D-printed mechanical components (planned)

The next milestone is to read physical probe position using the ESP32 and use it to control the virtual probe.

## Project Status

**Software simulation operational — hardware integration in progress.**

The simulator currently calculates and displays contact forces. It does not yet provide physical force feedback.

## Future Development

- ESP32-to-PC communication
- Physical probe position tracking
- Custom 3D-printed probe housing
- Actuator selection and integration
- Physical force-feedback control and safety testing

## Build Information

Developed on Windows using Visual Studio 2022, CMake, GLFW and CHAI3D 3.3.0.

Detailed build instructions will be added as the repository is organised.
