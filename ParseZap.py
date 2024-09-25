import argparse
import requests
import json
import os
import glob

from ParseXmlFolder import *
from ClusterReader import *

def download_zap_file(url, file_name):
    print(f"[DEBUG] Attempting to download file from URL: {url}")
    try:
        response = requests.get(url)
        response.raise_for_status()  # Ensure we got the file
        with open(file_name, 'wb') as file:
            file.write(response.content)
        print(f"[DEBUG] File downloaded successfully: {file_name}")
        return file_name
    except requests.exceptions.RequestException as e:
        print(f"[DEBUG] Failed to download file: {e}")
        return None

def parse_zap_file(file_name):
    print(f"[DEBUG] Attempting to parse file: {file_name}")
    try:
        with open(file_name, 'r') as file:
            data = json.load(file)  # Assuming it's a JSON file
            print(f"[DEBUG] File parsed successfully. Data loaded.")
            return data
    except json.JSONDecodeError as e:
        print(f"[DEBUG] Failed to parse JSON: {e}")
        return None
    except Exception as e:
        print(f"[DEBUG] Error reading file: {e}")
        return None

def find_ram_attributes_and_replace(data, replace=False):
    print(f"[DEBUG] Searching for attributes with storageOption 'RAM'.")
    ram_attributes = []
    modified = False
    
    # Traverse the JSON to find parent nodes and their attributes
    for endpointType in data.get('endpointTypes', []):  # Get 'endpointType' is the parent node section
        endpoint_id = endpointType.get('id')
        # Filter rootnode endpoint
        if endpoint_id == 1:
            continue

        for cluster in endpointType.get('clusters', []):  # Get 'clusters' is the parent node section
            cluster_name = cluster.get('name')
            cluster_code = cluster.get('code')

            if cluster_code > 0x7FFF: # Not standard cluster ID
                continue;

            for attribute in cluster.get('attributes', []):  # Iterate through the attributes
                attribute_code = attribute.get('code')  # Get the attribute's code
                # Filter global element
                if attribute_code >= 0xF000: # Golbal attribute 0xF000 - 0xFFFE
                    continue
                if attribute.get('storageOption') == 'RAM':  # Check if the storageOption is 'RAM'

                    attribute_name = attribute.get('name')  # Get the attribute's name

                    print(f"cluster_code = {cluster_code}, attribute_code={attribute_code}, attribute_name = {attribute_name}")
                    spec_xml = id2XmlMap[cluster_code]['file']
                    if not is_attribute_non_volatile(spec_xml, attribute_code):
                        print(f"\033[41m Ignore cluster: {cluster_name}, name:{attribute_name} \033[0m")
                        continue

                    print(f"\033[44m [DEBUG] Found RAM attribute: Parent Code: {cluster_code}, {cluster_name}, Attribute Code: {attribute_code}, Attribute Name: {attribute_name} \033[0m")
                    ram_attributes.append({
                        "cluster_code": cluster_code,
                        "cluster_name" : cluster_name,
                        "attribute_code": attribute_code,
                        "attribute_name": attribute_name,
                        "attribute": attribute
                    })
                    # Replace RAM to NVM
                    if replace:
                        attribute['storageOption'] = 'NVM'
                        modified = True
    
    print(f"[DEBUG] Found {len(ram_attributes)} attributes with storageOption 'RAM'.")
    for entry in ram_attributes:
        print(f"Parent Code: {entry['cluster_code']}, Attribute Code: {entry['attribute_code']}")
        print(json.dumps(entry['attribute'], indent=4))

    return modified

def process_zap_file(input_file, in_place):
    # Check if it's a URL or a local file
    if input_file.startswith("http://") or input_file.startswith("https://"):
        print(f"[DEBUG] Detected URL input: {input_file}")
        local_file_name = "downloaded_zap_file.zap"
        input_file = download_zap_file(input_file, local_file_name)
        if not input_file:
            print(f"[DEBUG] Exiting due to failed file download.")
            return
    else:
        print(f"[DEBUG] Detected local file input: {input_file}")
        if not os.path.isfile(input_file):
            print(f"[DEBUG] Error: The file {input_file} does not exist.")
            return
    
    # Parse the file and find RAM attributes
    parsed_data = parse_zap_file(input_file)
    if parsed_data:
        print(f"[DEBUG] Modifying storageOption from 'RAM' to 'NVM' in local file: {input_file}")
        modified = find_ram_attributes_and_replace(parsed_data, True)

        if modified:
            # If it's a local file, modify the storageOption and save it
            if os.path.isfile(input_file):

                if in_place:
                    # Save the modified JSON back to the original file
                    modified_file = input_file
                    print(f"[DEBUG] Saving in place: {modified_file}")
                else:
                    # Save the modified JSON back to the file (or to a new file)
                    modified_file = input_file.replace(".zap", "_modified.zap")
                    print(f"[DEBUG] Saving modified file as: {modified_file}")

                with open(modified_file, 'w') as file:
                    json.dump(parsed_data, file, indent=2)
                print(f"[DEBUG] File saved successfully.")
            else:
                # Handle case where it's a URL (output RAM attributes, don't modify)
                print(f"[DEBUG] Not local file, unable to modify")
        else:
            print(f"[DEBUG] No modifications were needed.")
    else:
        print(f"[DEBUG] Failed to parse the .zap file.")

def process_directory(directory, in_place):
    # Find all *.zap files in the directory
    print(f"[DEBUG] Processing all *.zap files in directory: {directory}")
    zap_files = glob.glob(os.path.join(directory, "*.zap"))
    if not zap_files:
        print(f"[DEBUG] No .zap files found in directory: {directory}")
        return
    
    print(f"[DEBUG] Found {len(zap_files)} .zap files.")
    for zap_file in zap_files:
        print(f"[DEBUG] Processing file: {zap_file}")
        process_zap_file(zap_file, in_place)


if __name__ == "__main__":
    # Command-line argument parsing
    parser = argparse.ArgumentParser(description="Parse a .zap file or a directory of .zap files and find/modify attributes with storageOption 'RAM'.")
    
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-f", "--file", help="Process a single zap file (local or remote URL).")
    group.add_argument("-d", "--directory", help="Process all *.zap files in a specific directory.")

    parser.add_argument("-s", "--spec", help="The folder path where spec xml files to be loaded", required=True)
    parser.add_argument("-i", "--in-place", action="store_true", help="Modify the files in place instead of creating a new file.", default=False)
  
    args = parser.parse_args()

    parse_xml_files_in_folder(args.spec)
    print(f"id2XmlMap: {id2XmlMap}")

    # Process the provided zap file or directory
    if args.file:
        print(f"[DEBUG] Starting process for file: {args.file}")
        process_zap_file(args.file, args.in_place)
    elif args.directory:
        print(f"[DEBUG] Starting process for directory: {args.directory}")
        process_directory(args.directory, args.in_place)

    print(f"[DEBUG] Process complete.")

