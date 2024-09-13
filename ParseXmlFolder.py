import os
import sys
import xml.etree.ElementTree as ET

id2XmlMap = []


def parse_xml_files_in_folder(folder_path):
    print(f"DEBUG: Starting to parse XML files in folder: {folder_path}")  

    for filename in os.listdir(folder_path):
        if filename.endswith('.xml'):
            full_path = os.path.join(folder_path, filename)
            print(f"DEBUG: Processing file: {filename}") 

            try:
                tree = ET.parse(full_path)
                root = tree.getroot()
                print(f"DEBUG: Successfully parsed {filename}")

                # Now you can work with the 'root' element to extract data
                # For example, to print all child elements:
                for child in root:
                    print(child.tag, child.attrib)

                    if child.tag != 'clusterIds':
                        continue

                    clusterIdSet = child.findall('clusterId')
                    print(clusterIdSet)
             
                    for clusterId in clusterIdSet:
                        clusterIdMap = {'id':clusterId.get('id'), 'name':clusterId.get('name'), 'file':full_path}

                        id2XmlMap.append(clusterIdMap)

                    print(f"DEBUG: Successfully parsed {filename}")

            except ET.ParseError as e:
                print(f"ERROR: Error parsing {filename}: {e}")

            print(clusterIdMap)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} folder_path")
        print(f"e.g.   python {sys.argv[0]} data_model/1.3/clusters")
        sys.exit(1)

    folder_path = sys.argv[1]
    parse_xml_files_in_folder(folder_path)
