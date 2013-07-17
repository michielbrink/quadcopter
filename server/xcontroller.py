#not my work, it's from https://github.com/morowinder/xcontroller

import pygame

import subprocess
import os
import atexit

proc = subprocess.Popen('xboxdrv', stdout=open(os.devnull, 'wb'))
atexit.register(proc.kill)



pygame.init()
pygame.joystick.init()


  def axis_deadzone(self, x):
    if x < 0:
      return int(round((1 * min(0, x + self.deadzone)) / (1 - self.deadzone) * 100))
    return int(round((1 * max(0, x - self.deadzone)) / (1 - self.deadzone) * 100))

  def trigger_deadzone (self, x):
    return  int(round((x + 1) / 2 * 100))

  def l_x(self):
    for event in pygame.event.get():
      None
    return (self.axis_deadzone(self.controller.get_axis(0)))
  def l_y(self):
    for event in pygame.event.get():
      None
    return (self.axis_deadzone(self.controller.get_axis(1)))

  def r_x(self):
    for event in pygame.event.get():
      None
    return (self.axis_deadzone(self.controller.get_axis(3)))
  def r_y(self):
    for event in pygame.event.get():
      None
    return (self.axis_deadzone(self.controller.get_axis(4)))



  def l_t(self):
    for event in pygame.event.get():
      None
    return (self.trigger_deadzone(self.controller.get_axis(2)))
  def r_t(self):
    for event in pygame.event.get():
      None
    return (self.trigger_deadzone(self.controller.get_axis(5)))


  def l_b(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(4)
  def r_b(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(5)

  def h_u(self):
    for event in pygame.event.get():
      None
    if self.controller.get_hat(0) == (0,1):
      return 1
    else:
      return 0
  def h_d(self):
    for event in pygame.event.get():
      None
    if self.controller.get_hat(0) == (0,-1):
      return 1
    else:
      return 0
  def h_l(self):
    for event in pygame.event.get():
      None
    if self.controller.get_hat(0) == (-1,0):
      return 1
    else:
      return 0
  def h_r(self):
    for event in pygame.event.get():
      None
    if self.controller.get_hat(0) == (1,0):
      return 1
    else:
      return 0

  def b_a(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(0)
  def b_b(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(1)
  def b_x(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(2)
  def b_y(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(3)

  def b_start(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(7)
  def b_back(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(6)
  def b_xbox(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(8)
  def b_l(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(9)
  def b_r(self):
    for event in pygame.event.get():
      None
    return self.controller.get_button(10)





# clock = pygame.time.Clock()


# pad1 = get_controller(0)

# while True:
  
#   # print("X: " + str(pad1.l_x()) + " Y: " + str(pad1.l_y()))
#   # print("X: " + str(pad1.r_x()) + " Y: " + str(pad1.r_y()))
#   # print ("LT: " + str(pad1.l_t()) + " RT: " + str(pad1.r_t()))
#   # print ("up: " + str(pad1.h_u()) + " down: " + str(pad1.h_d()) + " left: " + str(pad1.h_l()) + " right: " + str(pad1.h_r()) )
#   # print ("A: " + str(pad1.b_a()) + " B: " + str(pad1.b_b()) + " X: " + str(pad1.b_x()) + " Y: " + str(pad1.b_y()) )
#   # print ("START: " + str(pad1.b_start()) + " BACK: " + str(pad1.b_back()) + " XBOX: " + str(pad1.b_xbox()))
#   # print ("LEFT: " + str(pad1.b_l()) + " RIGHT: " + str(pad1.b_r()))
#   # print (pad1.l_x())
#   clock.tick(10)






