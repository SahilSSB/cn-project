from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import RemoteController
from mininet.cli import CLI

class MyTopo( Topo ):
    def build( self ):
        server = self.addHost( 'h1' )
        client1 = self.addHost( 'h2' )
        client2 = self.addHost( 'h3' )

        switch = self.addSwitch( 's1' )

        self.addLink( server, switch )
        self.addLink( client1, switch )
        self.addLink( client2, switch )

if __name__ == '__main__':
    topo = MyTopo()
    net = Mininet(topo=topo, controller=lambda name: RemoteController(name, ip='127.0.0.1'))
    net.start()
    
    print("Testing basic connectivity...")
    net.pingAll()
    
    CLI(net)
    net.stop()
