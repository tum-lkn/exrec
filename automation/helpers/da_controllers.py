import os

import global_config
from entities import StaticListDemandAwareController, ScheduledChangeDemandAwareController


def get_staticlist_x4_1da_all_flows(servers):
    return StaticListDemandAwareController(
        address=global_config.DA_CTR_HOST,
        git_path=global_config.GIT_PATH,
        list_of_links=[
                          {
                              'action': 0,
                              'src': s.tor_id,
                              'cache_id': 0,
                              'dst': d.tor_id,
                              'port': 0
                          }
                          for i, (s, d) in enumerate([(servers[-1], servers[0]), (servers[1], servers[2])])
                      ] + [
                          # Add cache rules for backflows...
                          {
                              'action': 0,
                              'src': d.tor_id,
                              'cache_id': 0,
                              'dst': s.tor_id,
                              'port': 0
                          }
                          for i, (s, d) in enumerate([(servers[-1], servers[0]), (servers[1], servers[2])])
                      ],
        interface="enp1s0"
    )


def get_staticlist_x4_2da_all_flows(servers):
    return StaticListDemandAwareController(
        address=global_config.DA_CTR_HOST,
        git_path=global_config.GIT_PATH,
        list_of_links=[
                          {
                              'action': 0,
                              'src': s.tor_id,
                              'cache_id': i % 2,
                              'dst': d.tor_id,
                              'port': 0
                          }
                          for i, (s, d) in enumerate(zip(servers[-1:] + servers[:-1], servers))
                      ] + [
                          # Add cache rules for backflows...
                          {
                              'action': 0,
                              'src': d.tor_id,
                              'cache_id': i % 2,
                              'dst': s.tor_id,
                              'port': 0
                          }
                          for i, (s, d) in enumerate(zip(servers[-1:] + servers[:-1], servers))
                      ],
        interface="enp1s0"
    )


def get_staticlist_x4_3da_all_flows(servers):
    links = [(0, servers[3], servers[0]), (0, servers[1], servers[2]),
             (1, servers[0], servers[1]), (1, servers[2], servers[3]),
             (2, servers[0], servers[2]), (2, servers[1], servers[3])]
    return StaticListDemandAwareController(
        address=global_config.DA_CTR_HOST,
        git_path=global_config.GIT_PATH,
        list_of_links=[
                          {
                              'action': 0,
                              'src': s.tor_id,
                              'cache_id': i,
                              'dst': d.tor_id,
                              'port': 0
                          }
                          for i, s, d in links
                      ] + [
                          # Add cache rules for backflows...
                          {
                              'action': 0,
                              'src': d.tor_id,
                              'cache_id': i,
                              'dst': s.tor_id,
                              'port': 0
                          }
                          for i, s, d in links
                      ],
        interface="enp1s0"
    )


# TODO add cache ctr for x4 3 DO -- currently set manually


def get_staticlist_x8_1da_all_flows(servers):
    ocs_pw = os.getenv('OCS_PW', None)
    if ocs_pw is None:
        print("OCS_PW not set. Run 'export OCS_PW=...'")
        raise RuntimeError("OCS_PW not found")
    return ScheduledChangeDemandAwareController(
        address=global_config.DA_CTR_HOST,
        git_path=global_config.GIT_PATH,
        t=0,
        list_of_links_t0=[
                             {
                                 'action': 0,
                                 'src': s.tor_id,
                                 'cache_id': 0,
                                 'dst': d.tor_id,
                                 'port': 0
                             }
                             for i, (s, d) in enumerate([(servers[7], servers[0]), (servers[1], servers[2]),
                                                         (servers[3], servers[4]), (servers[5], servers[6])])
                         ] + [
                             # Add cache rules for backflows...
                             {
                                 'action': 0,
                                 'src': d.tor_id,
                                 'cache_id': 0,
                                 'dst': s.tor_id,
                                 'port': 0
                             }
                             for i, (s, d) in enumerate([(servers[7], servers[0]), (servers[1], servers[2]),
                                                         (servers[3], servers[4]), (servers[5], servers[6])])
                         ],
        list_of_links_t1=None,
        clknet_interface="enp1s0",
        ocs_config={
            'host': global_config.OCS_HOST_NAME,
            'port': 830,
            'user': 'admin',
            'password': ocs_pw
        },
        tor_to_port="/root/mappings/tor_port_mapping_2rotor_2cache_all.json"
    )


def get_staticlist_x8_1da(servers):
    ocs_pw = os.getenv('OCS_PW', None)
    if ocs_pw is None:
        print("OCS_PW not set. Run 'export OCS_PW=...'")
        raise RuntimeError("OCS_PW not found")
    return ScheduledChangeDemandAwareController(
        address=global_config.DA_CTR_HOST,
        git_path=global_config.GIT_PATH,
        t=0,
        list_of_links_t0=[
                             {
                                 'action': 0,
                                 'src': s.tor_id,
                                 'cache_id': 0,
                                 'dst': d.tor_id,
                                 'port': 60000 + i
                             }
                             for i, (s, d) in enumerate(
                [(servers[7], servers[0]), (servers[1], servers[2]), (servers[3], servers[4]),
                 (servers[5], servers[6])])
                         ] + [
                             # Add cache rules for backflows...
                             {
                                 'action': 0,
                                 'src': d.tor_id,
                                 'cache_id': 0,
                                 'dst': s.tor_id,
                                 'port': 60000 + i
                             }
                             for i, (s, d) in enumerate(
                [(servers[7], servers[0]), (servers[1], servers[2]), (servers[3], servers[4]),
                 (servers[5], servers[6])])
                         ],
        list_of_links_t1=None,
        clknet_interface="enp1s0",
        ocs_config={
            'host': global_config.OCS_HOST_NAME,
            'port': 830,
            'user': 'admin',
            'password': ocs_pw
        },
        tor_to_port="/root/mappings/tor_port_mapping_2rotor_2cache_all.json"
    )


def get_static_x8_3do(servers):
    """
    Set OCS for 3 DO case -- statically connect third port to EPS
    :param servers:
    :return:
    """
    ocs_pw = os.getenv('OCS_PW', None)
    if ocs_pw is None:
        print("OCS_PW not set. Run 'export OCS_PW=...'")
        raise RuntimeError("OCS_PW not found")
    return ScheduledChangeDemandAwareController(
        address=global_config.DA_CTR_HOST,
        git_path=global_config.GIT_PATH,
        t=0,
        list_of_links_t0=[
                             {
                                 'action': 0,
                                 'src': s.tor_id,
                                 'cache_id': 0,
                                 'dst': d,
                                 'port': 0
                             } for i, (s, d) in enumerate(zip(servers, [f"chelsea{i}" for i in range(25, 33)]))
                         ] + [
                             # Add cache rules for backflows...
                             {
                                 'action': 0,
                                 'src': d,
                                 'cache_id': 0,
                                 'dst': s.tor_id,
                                 'port': 0
                             } for i, (s, d) in enumerate(zip(servers, [f"chelsea{i}" for i in range(25, 33)]))
                         ],
        list_of_links_t1=None,
        clknet_interface="enp1s0",
        ocs_config={
            'host': global_config.OCS_HOST_NAME,
            'port': 830,
            'user': 'admin',
            'password': ocs_pw
        },
        tor_to_port="/root/mappings/tor_port_mapping_3rotor_all.json",
        ctr_type="only_ocs"
    )
