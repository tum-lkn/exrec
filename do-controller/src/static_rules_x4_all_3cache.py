from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.ofproto import ofproto_v1_3
from ryu.lib.packet import packet
from ryu.lib.packet import ethernet


MATCHINGS = [
    (9, 13),
    (13, 9),
    (11, 15),
    (15, 11)
]

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
        for in_port, out_port in MATCHINGS:
            match = parser.OFPMatch(
                        in_port=in_port+PORT_OFFSET
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
