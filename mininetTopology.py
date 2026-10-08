from mininet.topo import Topo

class MyTopo( Topo ):
    "Some topology"

    def build ( self ):
        server = self.addHost( 'h1' )
        client1 = self.addHost( 'h2' )
        client2 = self.addHost( 'h3' )

        switch = self.addSwitch( 's1' )

        self.addLink( server, switch )
        self.addLink( client1, switch )
        self.addLink( client2, switch )

topos = { 'mytopo': ( lambda: MyTopo() ) }
