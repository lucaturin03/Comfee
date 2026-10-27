#include "HomeSpan.h"
#include <Appliance/AirConditioner/AirConditioner.h>

using namespace dudanov::midea::ac;

#define PIN_UART_RX 4   
#define PIN_UART_TX 5

AirConditioner ac;

struct Dev_HeaterCooler;
Dev_HeaterCooler *g_dev = nullptr;

struct Dev_HeaterCooler : Service::HeaterCooler {
  SpanCharacteristic *active;
  SpanCharacteristic *currentTemp;
  SpanCharacteristic *currentState;
  SpanCharacteristic *targetState;
  SpanCharacteristic *coolTemp;
  SpanCharacteristic *heatTemp;

  Dev_HeaterCooler() : Service::HeaterCooler() {
    active       = new Characteristic::Active(0);
    currentTemp  = new Characteristic::CurrentTemperature(20);
    currentState = new Characteristic::CurrentHeaterCoolerState(0);
    targetState  = new Characteristic::TargetHeaterCoolerState(2);
    coolTemp     = new Characteristic::CoolingThresholdTemperature(24);
    coolTemp->setRange(17, 30, 1);
    heatTemp     = new Characteristic::HeatingThresholdTemperature(20);
    heatTemp->setRange(17, 30, 1);
    g_dev = this;
  }

  void syncFromAc() {
    bool power = ac.getPowerState();
    active->setVal(power ? 1 : 0);

    float indoor = ac.getIndoorTemp();
    if (!isnan(indoor) && indoor > -40.0f && indoor < 80.0f)
      currentTemp->setVal(indoor);

    if (!power) {
      currentState->setVal(0);
      return;
    }

    Mode m = ac.getMode();
    float setpoint = ac.getTargetTemp();
    float room = currentTemp->getVal<float>();

    switch (m) {
      case MODE_COOL:
        targetState->setVal(2);
        coolTemp->setVal(setpoint);
        currentState->setVal(room > setpoint + 0.3f ? 3 : 1);
        break;
      case MODE_HEAT:
        targetState->setVal(1);
        heatTemp->setVal(setpoint);
        currentState->setVal(room < setpoint - 0.3f ? 2 : 1);   
        break;
      case MODE_AUTO:
        targetState->setVal(0);
        if      (room > setpoint + 0.5f) currentState->setVal(3);
        else if (room < setpoint - 0.5f) currentState->setVal(2);
        else                              currentState->setVal(1);
        break;
      default:
        currentState->setVal(1);
        break;
    }
  }

  boolean update() override {
    int isActive = active->updated() ? active->getNewVal<int>() : active->getVal<int>();

    if (!isActive) {
      ac.setPowerState(false);
      currentState->setVal(0);
      return true;
    }

    int mode = targetState->updated() ? targetState->getNewVal<int>() : targetState->getVal<int>();

    Control ctrl;
    switch (mode) {
      case 0: {
        ctrl.mode = MODE_AUTO;
        float cool = coolTemp->updated() ? coolTemp->getNewVal<float>() : coolTemp->getVal<float>();
        float heat = heatTemp->updated() ? heatTemp->getNewVal<float>() : heatTemp->getVal<float>();
        ctrl.targetTemp = (cool + heat) / 2.0f;
        break;
      }
      case 1:
        ctrl.mode = MODE_HEAT;
        ctrl.targetTemp = heatTemp->updated() ? heatTemp->getNewVal<float>() : heatTemp->getVal<float>();
        break;
      case 2:
        ctrl.mode = MODE_COOL;
        ctrl.targetTemp = coolTemp->updated() ? coolTemp->getNewVal<float>() : coolTemp->getVal<float>();
        break;
    }
    ctrl.fanMode = FAN_AUTO;

    ac.setPowerState(true);
    ac.control(ctrl);
    return true;
  }
};

void onAcStateChange() {
  if (g_dev) g_dev->syncFromAc();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial1.begin(9600, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
  ac.setStream(&Serial1);
  ac.addOnStateCallback(onAcStateChange);
  ac.setup();

  homeSpan.setPairingCode("02052003");
  homeSpan.begin(Category::AirConditioners, "Climatizzatore");

  new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name("Climatizzatore");
    new Characteristic::Manufacturer("Comfee");
    new Characteristic::Model("SuperMini C3 UART");
    new Dev_HeaterCooler();
}

void loop() {
  homeSpan.poll();
  ac.loop();        
}