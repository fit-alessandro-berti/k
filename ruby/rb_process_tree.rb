#!/usr/bin/env ruby

require 'rexml/document'
include REXML

# Class representing a node in the process tree
class ProcessNode
  attr_accessor :id, :name, :type, :children

  def initialize(id, name, type)
    @id = id              # Unique identifier of the node
    @name = name          # Name of the node
    @type = type          # Type of the node (e.g., 'and', 'sequence', 'manualTask')
    @children = []        # Children of this node
  end

  # Add a child node
  def add_child(child_node)
    @children << child_node
  end

  # Remove a child node
  def remove_child(child_node)
    @children.delete(child_node)
  end
end

# Class representing a parentsNode element (edge) in the process tree
class ParentsNode
  attr_accessor :id, :sourceId, :targetId

  def initialize(id, sourceId, targetId)
    @id = id              # Unique identifier of the parentsNode
    @sourceId = sourceId  # ID of the parent node
    @targetId = targetId  # ID of the child node
  end
end

if __FILE__ == $0
	input_file = ARGV[0]
	output_file = ARGV[1]
	
	# Main script
	if ARGV.length != 2
	  puts "Usage: ruby process_tree.rb input.ptml output.ptml"
	  exit
	end

	# Parse the input PTML file
	file = File.open(input_file)
	doc = Document.new(file)
	file.close

	# Get the processTree element
	process_tree_elem = doc.get_elements('/ptml/processTree').first

	# Extract root ID and attributes
	root_id = process_tree_elem.attributes['root']
	process_tree_attributes = {}
	process_tree_elem.attributes.each do |attr_name, attr_value|
	  process_tree_attributes[attr_name] = attr_value
	end

	nodes = {}          # Hash to store nodes by their IDs
	parents_nodes = []  # Array to store parentsNode elements

	# Parse node elements and create ProcessNode instances
	process_tree_elem.elements.each do |element|
	  case element.name
	  when 'and', 'or', 'xor', 'sequence', 'xorLoop', 'loop', 'manualTask', 'automaticTask'
		id = element.attributes['id']
		name = element.attributes['name']
		type = element.name
		node = ProcessNode.new(id, name, type)
		nodes[id] = node
	  end
	end

	# Parse parentsNode elements to build relationships
	process_tree_elem.elements.each('parentsNode') do |element|
	  id = element.attributes['id']
	  sourceId = element.attributes['sourceId']
	  targetId = element.attributes['targetId']

	  parents_node = ParentsNode.new(id, sourceId, targetId)
	  parents_nodes << parents_node

	  # Establish parent-child relationship
	  parent_node = nodes[sourceId]
	  child_node = nodes[targetId]

	  if parent_node && child_node
		parent_node.add_child(child_node)
	  else
		puts "Warning: Could not find parent node #{sourceId} or child node #{targetId}."
	  end
	end

	# Now the process tree data structure is built
	# You can manipulate the tree here if needed (e.g., insert or remove nodes)

	# Export the process tree back to PTML format
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

	puts "Process tree has been exported to #{output_file}."
end