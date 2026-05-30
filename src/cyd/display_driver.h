#pragma once
// =============================================================================
//  display_driver.h / display_driver.cpp
//  LovyanGFX configuration for the 7" Sunton ESP32-S3 (800×480, RGB parallel).
//  Registers the LVGL flush callback and tick timer.
//
//  Adjust pin definitions below for your exact board revision.
//  Sunton 7" (ESP32-S3R8N16) typical RGB pinout:
//    R[4:0] = GPIO 11,12,13,14,0
//    G[5:0] = GPIO 8,20,3,46,9,10
//    B[4:0] = GPIO 4,5,6,7,15
//    HSYNC=39, VSYNC=41, PCLK=40, DE=42, BL=2
// =============================================================================

void displayDriver_init();
