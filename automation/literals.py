# vIPs
VIRTUAL_IPS_BASE = "10.5.0."

IP_TO_MAC_MAPPING = {
    "10.5.0.1": "02:da:e1:00:00:01",
    "10.5.0.2": "02:da:e1:00:00:02",
    "10.5.0.3": "02:ba:57:1a:20:01",
    "10.5.0.4": "02:ba:57:1a:20:02",
    "10.5.0.5": "02:ca:a2:00:00:01",
    "10.5.0.6": "02:ca:a2:00:00:02",
    "10.5.0.7": "02:ba:11:ac:00:01",
    "10.5.0.8": "02:ba:11:ac:00:02",
}

VM_TYPE_VAGRANT = "vagrant"
VM_TYPE_VAGRANT_ML_GPU = "vagrant-ml-gpu"

CACHE_CTR_SIMPLE = "simple"
CACHE_CTR_ONLY_OCS = "only_ocs"

INDIRECT_MODE_ONLY_DIRECT = 0
INDIRECT_MODE_MAX = 1
INDIRECT_MODE_FIXED = 2
