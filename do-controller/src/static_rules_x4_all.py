from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.ofproto import ofproto_v1_3
from ryu.lib.packet import packet
from ryu.lib.packet import ethernet


MATCHINGS = {
    2: {
        1: 3, 3: 1, 5: 7, 7: 5,
        9: 15, 11: 13, 13: 11, 15: 9
    },
    3: {
        1: 7, 3: 5, 5: 3, 7: 1,
        9: 13, 11: 15, 13: 9, 15: 11
    },
    4: {
        1: 5, 3: 7, 5: 1, 7: 3,
        9: 11, 11: 9, 13: 15, 15: 13
    }
}

PORT_OFFSET = 80


class ExampleSwitch13(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(ExampleSwitch13, self).__init__(*args, **kwargs)
        # initialize mac address table.
        self.mac_to_port = {}

    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        
        print("New switch, deploying flows...")

        match = parser.OFPMatch()
        actions = []
        flow_mod = parser.OFPFlowMod(datapath, command=ofproto.OFPFC_DELETE, match=match, instructions=actions)
        datapath.send_msg(flow_mod)


        # install the table-miss flow entry.
        for vid, port_dict in MATCHINGS.items():
            for in_port, out_port in port_dict.items():
                match = parser.OFPMatch(
                            in_port=in_port+PORT_OFFSET,
                            vlan_vid=(0x1000 | vid)
                        )
                actions = [
                         parser.OFPActionPopVlan(), 
                        parser.OFPActionOutput(out_port+PORT_OFFSET)
                ]
                self.add_flow(datapath, 0, match, actions)

    def add_flow(self, datapath, priority, match, actions):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        # construct flow_mod message and send it.
        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS,
                                                          actions)]
        mod = parser.OFPFlowMod(datapath=datapath, priority=priority,
                match=match, instructions=inst
        )
        datapath.send_msg(mod)
