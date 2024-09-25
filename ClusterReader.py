import requests
from bs4 import BeautifulSoup
import sys

def is_attribute_non_volatile(source, attribute_id):
    """
    Checks if the attribute with the given hexadecimal ID has 'nonVolatile' persistence in the XML.
    Also prints the attribute name if found.

    Args:
        source: Either a URL (string) or a file path (string) to the XML file.
        attribute_id: The hexadecimal ID of the attribute to check (e.g., 0x0000).

    Returns:
        True if the attribute has 'nonVolatile' persistence, False otherwise.
    """

    if source.startswith("http://") or source.startswith("https://"):
        response = requests.get(source)
        response.raise_for_status()
        content = response.content

        # Debug: Check the response status code
        print("Response status code:", response.status_code)

    else:
        with open(source, 'r') as file:
            content = file.read()

    print(f"source = {source}")
    print(f"attribute_id = {attribute_id}")
    soup = BeautifulSoup(content, 'lxml-xml')

    # Debug: Print the parsed XML structure (for a quick visual check)
    # print("Parsed XML:", soup.prettify())

    # Find the attribute with the given ID (convert hex ID from XML to integer for comparison)
    for attribute in soup.find_all('attribute', {'id': lambda x: x is not None and int(x, 16) == attribute_id}):

        mandatoryConform = attribute.find('mandatoryConform')

        # Ignore conformance which is Zigbee
        if mandatoryConform:
            condition = mandatoryConform.find('condition')
            if condition and condition.get('name') == 'Zigbee':
                print(f"Ignore conformance which is Zigbee")
                continue

        quality_node = attribute.find('quality')

        # Debug: Check if the quality node was found
        print("Quality node found:", quality_node is not None)

        if quality_node and quality_node.get('persistence') == 'nonVolatile':
            print(f"Attribute name: {attribute['name']}")
            return True

    return False

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: python {sys.argv[0]} <source> <attribute_id>")
        print(f"e.g.   python {sys.argv[0]} data_model/1.3/clusters/OnOff.xml 0x0000")
        sys.exit(1)

    source = sys.argv[1]
    attribute_id_to_check = int(sys.argv[2], 16)

    result = is_attribute_non_volatile(source, attribute_id_to_check)
    print(f"Attribute with ID '0x{attribute_id_to_check:04X}' is non-volatile: {result}")
