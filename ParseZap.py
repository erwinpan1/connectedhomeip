import argparse
import requests
import json
import os

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

def find_ram_attributes_with_codes(data):
    print(f"[DEBUG] Searching for attributes with storageOption 'RAM'.")
    ram_attributes = []
    
    # Traverse the JSON to find parent nodes and their attributes
    for endpointType in data.get('endpointTypes', []):  # Get 'endpointType' is the parent node section

        endpoint_id = endpointType.get('id')

        # Filter rootnode endpoint
        if endpoint_id == 0:
            continue

        for cluster in endpointType.get('clusters', []):  # Get 'clusters' is the parent node section

            cluster_name = cluster.get('name')
            cluster_code = cluster.get('code')

            for attribute in cluster.get('attributes', []):  # Iterate through the attributes

                attribute_code = attribute.get('code')  # Get the attribute's code
                # Filter global element
                if attribute_code >= 0xF000: # Golbal attribute 0xF000 - 0xFFFE
                    continue

                if attribute.get('storageOption') == 'RAM':  # Check if the storageOption is 'RAM'
                    attribute_name = attribute.get('name')  # Get the attribute's name
                    print(f"[DEBUG] Found RAM attribute: Parent Code: {cluster_code}, {cluster_name}, Attribute Code: {attribute_code}, Attribute Name: {attribute_name}")
                    ram_attributes.append({
                        "cluster_code": cluster_code,
                        "cluster_name" : cluster_name,
                        "attribute_code": attribute_code,
                        "attribute_name": attribute_name,
                        "attribute": attribute
                    })
    
    print(f"[DEBUG] Found {len(ram_attributes)} attributes with storageOption 'RAM'.")
    return ram_attributes

def modify_storage_option(data):
    print(f"[DEBUG] Modifying attributes with storageOption 'RAM' to 'NVRAM'.")
    modified = False
    # Traverse the JSON and modify attributes with "storageOption" as "RAM" to "NVRAM"
    for parent in data.get('nodes', []):
        for attribute in parent.get('attributes', []):
            if attribute.get('storageOption') == 'RAM':
                print(f"[DEBUG] Modifying attribute with code {attribute.get('code')} from 'RAM' to 'NVRAM'")
                attribute['storageOption'] = 'NVRAM'
                modified = True
    if modified:
        print(f"[DEBUG] Attributes modified successfully.")
    else:
        print(f"[DEBUG] No modifications were made (no attributes with 'RAM' found).")
    return modified

def process_zap_file(input_file):
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
        # If it's a local file, modify the storageOption and save it
        #if os.path.isfile(input_file):
        if False:
            print(f"[DEBUG] Modifying storageOption from 'RAM' to 'NVRAM' in local file: {input_file}")
            modified = modify_storage_option(parsed_data)
            if modified:
                # Save the modified JSON back to the file (or to a new file)
                modified_file = input_file.replace(".zap", "_modified.zap")
                print(f"[DEBUG] Saving modified file as: {modified_file}")
                with open(modified_file, 'w') as file:
                    json.dump(parsed_data, file, indent=4)
                print(f"[DEBUG] File saved successfully.")
            else:
                print(f"[DEBUG] No modifications were needed.")
        else:
            # Handle case where it's a URL (output RAM attributes, don't modify)
            ram_attributes = find_ram_attributes_with_codes(parsed_data)
            if ram_attributes:
                print("[DEBUG] Displaying RAM attributes found in the file:")
                for entry in ram_attributes:
                    print(f"Parent Code: {entry['cluster_code']}, Attribute Code: {entry['attribute_code']}")
                    print(json.dumps(entry['attribute'], indent=4))
            else:
                print(f"[DEBUG] No attributes with storageOption 'RAM' found.")
    else:
        print(f"[DEBUG] Failed to parse the .zap file.")

if __name__ == "__main__":
    # Command-line argument parsing
    parser = argparse.ArgumentParser(description="Parse a .zap file and find/modify attributes with storageOption 'RAM'.")
    parser.add_argument("zap_file", help="The URL or local path to the .zap file.")

    args = parser.parse_args()

    # Process the provided zap file (URL or local path)
    print(f"[DEBUG] Starting process for input file: {args.zap_file}")
    process_zap_file(args.zap_file)
    print(f"[DEBUG] Process complete.")

