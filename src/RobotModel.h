#pragma once
#include "Player.h"
#include "Renderer.h"

// VOLT: özgün tasarım, yordamsal (kodla üretilen) hümanoid robot.
// Turuncu-beyaz gövde, yüz siperliği, parlayan cam-mavisi göz/çekirdek, anten.
// Tüm uzuvlar hiyerarşik matrislerle (omuz -> dirsek -> el, kalça -> diz -> ayak) animasyonlanır.
void drawRobot(Renderer& r, const Player& p, float time);
