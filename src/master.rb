# オンボードLED点滅サンプル
#
# Raspberry Pi PicoのオンボードLEDを1秒間隔で点滅させる．

led = GPIO.new("LED", GPIO::OUT)

while true
  led.write(1)
  sleep(1)
  led.write(0)
  sleep(1)
end
