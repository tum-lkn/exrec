import os

import config
import preprocessing_functions as pre_f


pcap_files = pre_f.find_all_pcap_files_without_ftype_files(os.path.join(config.BASE_DATA_PATH, "scenario1_8tors"))
print(pcap_files)
num_pcaps = pre_f.parallel_pcap_parser(pcap_files, num_workers=16)

print(f"Parsed {num_pcaps} pcaps.")
