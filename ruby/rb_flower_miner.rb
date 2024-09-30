#!/usr/bin/env ruby

require_relative 'rb_xes'
require_relative 'rb_process_tree'

include REXML

# Check for the correct number of command-line arguments
if ARGV.length != 2
  puts "Usage: ruby #{__FILE__} input_file.xes output_file.ptml"
  exit 1
end

input_file = ARGV[0]
output_file = ARGV[1]

# Verify that the input file exists
unless File.exist?(input_file)
  puts "Error: Input file '#{input_file}' does not exist."
  exit 1
end

begin
  # Import the XES event log
  importer = XESImporterExporter.new
  puts "Importing from '#{input_file}'..."
  importer.import(input_file)
  puts "Successfully imported #{importer.traces.size} traces."
rescue => e
  puts "An error occurred while importing the XES file: #{e.message}"
  exit 1
end

# Extract the unique set of activities from the event log
activities_hash = {}
importer.traces.each do |trace|
  trace.each do |activity|
    activities_hash[activity] = true
  end
end
activities = activities_hash.keys
puts "Found #{activities.size} unique activities."

# Initialize ID counter for generating unique IDs
$id_counter = 0

# Helper method to generate unique IDs
def generate_id
  id = "n#{$id_counter}"
  $id_counter += 1
  id
end

# Create the process tree nodes and relationships
nodes = {}
parents_nodes = []

# Create root node (loop)
root_id = generate_id
root_node = ProcessNode.new(root_id, '', 'xorLoop')
nodes[root_id] = root_node

# Create 'Do' node (choice between activities)
do_id = generate_id
do_node = ProcessNode.new(do_id, '', 'xor')
nodes[do_id] = do_node

# Create 'Redo' node (invisible transition)
redo_id = generate_id
# Invisible task represented as an automaticTask with an empty name
redo_node = ProcessNode.new(redo_id, '', 'automaticTask')
nodes[redo_id] = redo_node

# Define relationships between root node and its children
pn_root_do = ParentsNode.new(generate_id, root_id, do_id)
pn_root_redo = ParentsNode.new(generate_id, root_id, redo_id)
parents_nodes << pn_root_do << pn_root_redo

# For each activity, create a node and connect it to the 'Do' node
activities.each do |activity|
  activity_id = generate_id
  activity_node = ProcessNode.new(activity_id, activity, 'manualTask')
  nodes[activity_id] = activity_node

  # Define the relationship between 'Do' node and activity node
  pn_do_activity = ParentsNode.new(generate_id, do_id, activity_id)
  parents_nodes << pn_do_activity
end

# Set the process tree attributes with the root ID
process_tree_attributes = { 'root' => root_id }

# Export the process tree to PTML format
begin
  output_doc = Document.new
  output_doc << XMLDecl.new('1.0', 'UTF-8')

  # Create root ptml element
  ptml_elem = output_doc.add_element('ptml')

  # Create processTree element with attributes
  process_tree_elem_out = ptml_elem.add_element('processTree')
  process_tree_attributes.each do |attr_name, attr_value|
    process_tree_elem_out.add_attribute(attr_name, attr_value)
  end

  # Add node elements to the processTree
  nodes.values.each do |node|
    node_elem = process_tree_elem_out.add_element(node.type)
    node_elem.add_attribute('id', node.id)
    node_elem.add_attribute('name', node.name || '')
  end

  # Add parentsNode elements to define the relationships
  parents_nodes.each do |pn|
    pn_elem = process_tree_elem_out.add_element('parentsNode')
    pn_elem.add_attribute('id', pn.id)
    pn_elem.add_attribute('sourceId', pn.sourceId)
    pn_elem.add_attribute('targetId', pn.targetId)
  end

  # Write the output document to the output file
  File.open(output_file, 'w') do |file|
    formatter = REXML::Formatters::Pretty.new(2) # Indent by 2 spaces
    formatter.compact = true                     # Remove unnecessary whitespace
    formatter.write(output_doc, file)
  end

  puts "Process tree has been exported to '#{output_file}'."
rescue => e
  puts "An error occurred while exporting the PTML file: #{e.message}"
  exit 1
end