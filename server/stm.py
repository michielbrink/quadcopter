PAYLOAD_LENGTH = 8
class stm_message:
    def __init__(self, command, payload):
        msg = []
        msg.append(command)
        msg += payload[:PAYLOAD_LENGTH]
        msg += [0] * ( PAYLOAD_LENGTH - len(payload) )
        par = reduce(lambda x, y: x^y, msg)
        msg.append(par)
        self.msg = [x for x in msg]

    def get_message(self):
        return self.msg

    def __repr__(self):
        return "--> %02X : %s : %02X" % (
            self.msg[0],  ' '.join(["%02X" % x for x in self.msg[1:-2]]), self.msg[-1] )

class stm_response:
    def __init__(self, message):
        print repr(message)
        self.msg = message
        cmd = message[0]
        payload = message[1:PAYLOAD_LENGTH+1]
        par = message[-1]
        par_check = reduce(lambda x, y: x^y, message)
    def __repr__(self):
        return "<-- %02X : %s : %02X" % (
            self.msg[0],  ' '.join(["%02X" % x for x in self.msg[1:-2]]), self.msg[-1] )


"""
# Voor motor-controls zijn het big-endian 16 bit integers
print "Alle motorem uit:"
m1 = stm_message(0x02, [0,0,0,0,0,0,0,0])
resp = spi.xfer2(m1.get_message())

time.sleep(2)

print "Alle motorem aan:"
m1 = stm_message(0x02, [200,0,200,0,200,0,200,0])
resp = spi.xfer2(m1.get_message())

time.sleep(3)

print "Motor1 harder aan:"
m1 = stm_message(0x02, [50,1,0,0,0,0,0,0])
resp = spi.xfer2(m1.get_message())

time.sleep(3)

print "Alle motorem weer uit:"
m1 = stm_message(0x02, [0,0,0,0,0,0,0,0])
resp = spi.xfer2(m1.get_message())
"""
