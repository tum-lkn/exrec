from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.ofproto import ofproto_v1_3
from ryu.lib.packet import packet
from ryu.lib.packet import ethernet

MATCHINGS = {
    2: {
        1: 2, 2: 1, 3: 4, 4: 3, 5: 6, 6: 5, 7: 8, 8: 7,
        9: 13, 10: 14, 11: 15, 12: 16, 13: 9, 14: 10, 15: 11, 16: 12,
        17: 24, 18: 21, 19: 22, 20: 23, 21: 18, 22: 19, 23: 20, 24: 17
    },
    3: {
        1: 3, 2: 4, 3: 1, 4: 2, 5: 7, 6: 8, 7: 5, 8: 6,
        9: 14, 10: 15, 11: 16, 12: 13, 13: 12, 14: 9, 15: 10, 16: 11,
        17: 18, 18: 17, 19: 20, 20: 19, 21: 22, 22: 21, 23: 24, 24: 23
    },
    4: {
        1: 4, 2: 3, 3: 2, 4: 1, 5: 8, 6: 7, 7: 6, 8: 5,
        9: 15, 10: 16, 11: 13, 12: 14, 13: 11, 14: 12, 15: 9, 16: 10,
        17: 19, 18: 20, 19: 17, 20: 18, 21: 23, 22: 24, 23: 21, 24: 22
    },
    5: {
        1: 5, 2: 6, 3: 7, 4: 8, 5: 1, 6: 2, 7: 3, 8: 4,
        9: 16, 10: 13, 11: 14, 12: 15, 13: 10, 14: 11, 15: 12, 16: 9,
        17: 20, 18: 19, 19: 18, 20: 17, 21: 24, 22: 23, 23: 22, 24: 21
    },
    6: {
        1: 6, 2: 7, 3: 8, 4: 5, 5: 4, 6: 1, 7: 2, 8: 3,
        9: 10, 10: 9, 11: 12, 12: 11, 13: 14, 14: 13, 15: 16, 16: 15,
        17: 21, 18: 22, 19: 23, 20: 24, 21: 17, 22: 18, 23: 19, 24: 20
    },
    7: {
        1: 7, 2: 8, 3: 5, 4: 6, 5: 3, 6: 4, 7: 1, 8: 2,
        9: 11, 10: 12, 11: 9, 12: 10, 13: 15, 14: 16, 15: 13, 16: 14,
        17: 22, 18: 23, 19: 24, 20: 21, 21: 20, 22: 17, 23: 18, 24: 19
    },
    8: {
        1: 8, 2: 5, 3: 6, 4: 7, 5: 2, 6: 3, 7: 4, 8: 1,
        9: 12, 10: 11, 11: 10, 12: 9, 13: 16, 14: 15, 15: 14, 16: 13,
        17: 23, 18: 24, 19: 21, 20: 22, 21: 19, 22: 20, 23: 17, 24: 18
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
