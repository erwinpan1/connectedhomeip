import os
import sys
import xml.etree.ElementTree as ET

name2XmlMap = {}
id2XmlMap = {} 
parentClusterMap = {}

def parse_xml_files_in_folder(folder_path):
    print(f"DEBUG: Starting to parse XML files in folder: {folder_path}")  

    for filename in os.listdir(folder_path):
        if filename.endswith('.xml'):
            full_path = os.path.join(folder_path, filename)
            print("==========================================================")
            print(f"DEBUG: Processing file: {filename}") 

            try:
                tree = ET.parse(full_path)
                root = tree.getroot()
                print(f"DEBUG: Successfully parsed {filename}")

                # If it is a derived cluster
                classification = root.find('classification')
                if classification is not None:
                    baseCluster = classification.get('baseCluster')
                else:
                    baseCluster = None


                # Find the clusterIds
                clusterIds = root.find('clusterIds')
                if clusterIds:
                    print(clusterIds)

                    clusterIdSet = clusterIds.findall('clusterId')
                    print(clusterIdSet)
             
                    for clusterId in clusterIdSet:
                        if baseCluster is not None:
                            parentClusterMap[ int(clusterId.get('id'), 16) ] = {'name':clusterId.get('name'), 'file':full_path, 'baseCluster': baseCluster}
                            print(f'Found cluster {clusterId} with parent cluster {baseCluster}')
                        elif clusterId.get('id') is not None:
                            id2XmlMap[ int(clusterId.get('id'), 16) ] = {'name':clusterId.get('name'), 'file':full_path}
                            print(f'Found cluster with id {clusterId}')
                        else:
                            print(f'Found cluster without id {clusterId}')

                        name2XmlMap[str(clusterId.get('name'))] = {'file':full_path}

            except ET.ParseError as e:
                print(f"ERROR: Error parsing {filename}: {e}")

    print(name2XmlMap)

    for key, value in parentClusterMap.items():
        print(f'Processing derived class: name: {value["name"]}, parent: {value["baseCluster"]}')
        id2XmlMap[key] = {'name': value['name'], 'file': name2XmlMap[value['baseCluster']]['file']}



if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} folder_path")
        print(f"e.g.   python {sys.argv[0]} data_model/1.3/clusters")
        sys.exit(1)

    folder_path = sys.argv[1]
    parse_xml_files_in_folder(folder_path)
    print(id2XmlMap)
