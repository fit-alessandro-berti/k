#!/usr/bin/env ruby

# Required modules
require_relative 'rb_xes'
require_relative 'rb_process_tree'
require 'set'
require 'securerandom'  # For generating unique IDs
require 'rexml/document'
include REXML

# Check command-line arguments
if ARGV.length != 2
  puts "Usage: ruby script.rb input_file.xes output_file.ptml"
  exit 1
end

input_file = ARGV[0]
output_file = ARGV[1]

# Create an instance of the XES importer/exporter
xes_importer = XESImporterExporter.new

# Import the XES file
begin
  xes_importer.import(input_file)
rescue => e
  puts "An error occurred during XES import: #{e.message}"
  exit 1
end

traces = xes_importer.traces

# Initialize DFG components
activities = Set.new
start_activities = Set.new
end_activities = Set.new
edges = Set.new

traces.each do |trace|
  next if trace.empty?

  activities.merge(trace)
  start_activities << trace.first
  end_activities << trace.last

  trace.each_cons(2) do |from_activity, to_activity|
    edges << [from_activity, to_activity]
  end
end

# Define the DFG class
class DFG
  attr_accessor :activities, :start_activities, :end_activities, :edges

  def initialize(activities, start_activities, end_activities, edges)
    @activities = activities
    @start_activities = start_activities
    @end_activities = end_activities
    @edges = edges
  end

  # Projection function: returns a new DFG projected on the given activities
	def project(activities_subset)
	  # Projected edges: edges where both 'from' and 'to' are in activities_subset
	  projected_edges = @edges.select { |from, to| activities_subset.include?(from) && activities_subset.include?(to) }

	  # Compute activities with incoming edges in the projected edges
	  activities_with_incoming_edges = projected_edges.map { |from, to| to }.to_set

	  # Compute activities with outgoing edges in the projected edges
	  activities_with_outgoing_edges = projected_edges.map { |from, to| from }.to_set

	  # Start activities are those with no incoming edges
	  projected_start_activities = activities_subset - activities_with_incoming_edges

	  # End activities are those with no outgoing edges
	  projected_end_activities = activities_subset - activities_with_outgoing_edges

	  DFG.new(activities_subset, projected_start_activities, projected_end_activities, projected_edges)
	end
end

# Create the DFG
dfg = DFG.new(activities, start_activities, end_activities, edges)

# Implement the Inductive Miner algorithm
def inductive_miner(dfg)
  # Base case: if DFG has only one activity
  if dfg.activities.size == 1
    activity = dfg.activities.to_a.first
    node = ProcessNode.new(SecureRandom.uuid, activity, 'manualTask')
    return node
  end

  # Try to detect sequence cut
  sequence_groups = detect_sequence_cut(dfg)
  if sequence_groups && sequence_groups.length > 1
    sequence_node = ProcessNode.new(SecureRandom.uuid, '', 'sequence')
    sequence_groups.each do |group|
      projected_dfg = dfg.project(group)
      child_node = inductive_miner(projected_dfg)
      sequence_node.add_child(child_node)
    end
    return sequence_node
  end

  # Try to detect XOR cut
  xor_groups = detect_xor_cut(dfg)
  if xor_groups && xor_groups.length > 1
    xor_node = ProcessNode.new(SecureRandom.uuid, '', 'xor')
    xor_groups.each do |group|
      projected_dfg = dfg.project(group)
      child_node = inductive_miner(projected_dfg)
      xor_node.add_child(child_node)
    end
    return xor_node
  end

  # Try to detect parallel cut
  parallel_groups = detect_parallel_cut(dfg)
  if parallel_groups && parallel_groups.length > 1
    and_node = ProcessNode.new(SecureRandom.uuid, '', 'and')
    parallel_groups.each do |group|
      projected_dfg = dfg.project(group)
      child_node = inductive_miner(projected_dfg)
      and_node.add_child(child_node)
    end
    return and_node
  end

  # Try to detect loop cut
  loop_groups = detect_loop_cut(dfg)
  if loop_groups && loop_groups.length > 1
    loop_node = ProcessNode.new(SecureRandom.uuid, '', 'xorLoop')

    # 'Do' group
    do_group = loop_groups.shift
    do_dfg = dfg.project(do_group)
    do_node = inductive_miner(do_dfg)

    # 'Redo' group (merge if multiple groups exist)
    if loop_groups.length == 1
      redo_dfg = dfg.project(loop_groups.first)
      redo_node = inductive_miner(redo_dfg)
    else
      redo_node = ProcessNode.new(SecureRandom.uuid, '', 'xor')
      loop_groups.each do |group|
        projected_dfg = dfg.project(group)
        child_node = inductive_miner(projected_dfg)
        redo_node.add_child(child_node)
      end
    end

    loop_node.add_child(do_node)
    loop_node.add_child(redo_node)
    return loop_node
  end

  # Base case: cannot detect any cuts, create flower model
  # Create 'xorLoop' node
  loop_node = ProcessNode.new(SecureRandom.uuid, '', 'xorLoop')

  # 'Do' node: XOR over all activities
  do_node = ProcessNode.new(SecureRandom.uuid, '', 'xor')
  dfg.activities.each do |activity|
    activity_node = ProcessNode.new(SecureRandom.uuid, activity, 'manualTask')
    do_node.add_child(activity_node)
  end

  # 'Redo' node: silent task
  redo_node = ProcessNode.new(SecureRandom.uuid, '', 'automaticTask')

  # Add 'do' and 'redo' nodes to loop_node
  loop_node.add_child(do_node)
  loop_node.add_child(redo_node)
  return loop_node
end

# Detect sequence cut
def detect_sequence_cut(dfg)
  # Step 1: Create a group per activity
  groups = {}
  dfg.activities.each do |activity|
    groups[activity] = [activity]
  end

  # Step 2: Merge pairwise reachable nodes
  reachability = compute_reachability(dfg)
  union_find = UnionFind.new(dfg.activities)

  dfg.activities.each do |a|
    reachability[a].each do |b|
	  if reachability[a].include?(b) && reachability[b].include?(a)
        union_find.union(a, b)
      end
	end
  end
  
  # Step 3: Merge pairwise unreachable nodes
  dfg.activities.each do |a|
    dfg.activities.each do |b|
      unless reachability[a].include?(b) || reachability[b].include?(a)
        union_find.union(a, b)
      end
    end
  end

  # Step 4: Sort the groups based on their reachability
  group_map = {}
  dfg.activities.each do |activity|
    root = union_find.find(activity)
    group_map[root] ||= Set.new
    group_map[root] << activity
  end

  groups = group_map.values

  # Attempt to sort the groups
  order = topological_sort_groups(groups, dfg)
  return order if order
  nil
end

# Compute reachability using DFS
def compute_reachability(dfg)
  reachability = {}
  adjacency = Hash.new { |hash, key| hash[key] = [] }
  dfg.edges.each do |from, to|
    adjacency[from] << to
  end
  dfg.activities.each do |activity|
    visited = Set.new
    stack = [activity]
    while !stack.empty?
      current = stack.pop
      adjacency[current].each do |neighbor|
        unless visited.include?(neighbor)
          visited << neighbor
          stack.push(neighbor)
        end
      end
    end
    reachability[activity] = visited
  end
  reachability
end

# UnionFind class for grouping
class UnionFind
  def initialize(elements)
    @parent = {}
    elements.each do |e|
      @parent[e] = e
    end
  end

  def find(e)
    while @parent[e] != e
      e = @parent[e]
    end
    e
  end

  def union(a, b)
    root_a = find(a)
    root_b = find(b)
    @parent[root_b] = root_a unless root_a == root_b
  end
end

# Topological sort groups
def topological_sort_groups(groups, dfg)
  group_order = []
  visited_groups = {}
  adjacency = {}
  groups.each do |group|
    group.each do |activity|
      adjacency[activity] = []
    end
  end
  dfg.edges.each do |from, to|
    group_from = groups.find { |g| g.include?(from) }
    group_to = groups.find { |g| g.include?(to) }
    if group_from != group_to
      adjacency[group_from] ||= Set.new
      adjacency[group_from] << group_to
    end
  end

  temp_mark = {}
  stack = []

  visit = lambda do |n|
    return false if temp_mark[n]
    return true if visited_groups[n]
    temp_mark[n] = true
    if adjacency[n]
      adjacency[n].each do |m|
        return false unless visit.call(m)
      end
    end
    temp_mark.delete(n)
    visited_groups[n] = true
    group_order << n
    true
  end

  groups.each do |group|
    return nil unless visit.call(group)
  end

  group_order.reverse!
  group_order
end

# Detect XOR cut
def detect_xor_cut(dfg)
  # Convert DFG edges to undirected edges
  undirected_edges = dfg.edges.to_a + dfg.edges.map { |from, to| [to, from] }
  # Build adjacency list
  adjacency = Hash.new { |hash, key| hash[key] = [] }
  undirected_edges.each do |from, to|
    adjacency[from] << to
    adjacency[to] << from
  end
  visited = {}
  components = []

  dfg.activities.each do |activity|
    next if visited[activity]
    # Start a new component
    component = Set.new
    queue = [activity]
    visited[activity] = true
    while !queue.empty?
      current = queue.shift
      component << current
      adjacency[current].each do |neighbor|
        unless visited[neighbor]
          visited[neighbor] = true
          queue << neighbor
        end
      end
    end
    components << component
  end

  if components.size > 1
    # Return the groups
    return components
  else
    return nil
  end
end

# Detect parallel cut
def detect_parallel_cut(dfg)
  activities = dfg.activities.to_a

  # Build conflict graph where edges represent directly follows relations in both directions
  conflict_graph = {}
  activities.each do |a|
    conflict_graph[a] = Set.new
  end

  # Build edges in the conflict graph
  activities.combination(2).each do |a, b|
    if dfg.edges.include?([a, b]) && dfg.edges.include?([b, a])
      conflict_graph[a] << b
      conflict_graph[b] << a
    end
  end

  # Graph coloring using a greedy algorithm
  coloring = {}
  used_colors = Set.new

  # Sort activities by the number of conflicts (descending) for better coloring
  activities.sort_by { |a| -conflict_graph[a].size }.each do |activity|
    # Colors of neighboring activities
    neighbor_colors = conflict_graph[activity].map { |neighbor| coloring[neighbor] }.compact.to_set

    # Assign the smallest available color
    color = (0..activities.size).find { |c| !neighbor_colors.include?(c) }
    coloring[activity] = color
    used_colors << color
  end

  # Group activities by color
  groups = {}
  coloring.each do |activity, color|
    groups[color] ||= Set.new
    groups[color] << activity
  end

  group_list = groups.values

  # Check that each group has at least one start activity and one end activity
  valid = true
  group_list.each do |group|
    group_start_activities = dfg.start_activities & group
    group_end_activities = dfg.end_activities & group
    unless !group_start_activities.empty? && !group_end_activities.empty?
      valid = false
      break
    end
  end

  # Check the required relations between groups
  if valid
    group_list.combination(2).each do |g1, g2|
      g1.each do |a|
        g2.each do |b|
          unless dfg.edges.include?([a, b]) && dfg.edges.include?([b, a])
            valid = false
            break
          end
        end
        break unless valid
      end
      break unless valid
    end
  end

  if valid && group_list.size > 1
    return group_list
  else
    return nil
  end
end

# Detect loop cut
def detect_loop_cut(dfg)
  # Step 1: Merge all start and end activities into one group ('do' group)
  do_group = dfg.start_activities | dfg.end_activities
  remaining_activities = dfg.activities - do_group

  # Step 2: Remove start/end activities from the DFG
  reduced_edges = dfg.edges.reject { |from, to| do_group.include?(from) || do_group.include?(to) }
  reduced_dfg = DFG.new(remaining_activities, Set.new, Set.new, reduced_edges)

  # Step 3: Detect connected components in the reduced graph
  undirected_edges = reduced_dfg.edges.to_a + reduced_dfg.edges.map { |from, to| [to, from] }
  adjacency = Hash.new { |hash, key| hash[key] = [] }
  undirected_edges.each do |from, to|
    adjacency[from] << to
    adjacency[to] << from
  end
  visited = {}
  components = []

  reduced_dfg.activities.each do |activity|
    next if visited[activity]
    # Start a new component
    component = Set.new
    queue = [activity]
    visited[activity] = true
    while !queue.empty?
      current = queue.shift
      component << current
      adjacency[current].each do |neighbor|
        unless visited[neighbor]
          visited[neighbor] = true
          queue << neighbor
        end
      end
    end
    components << component
  end

  # Step 4: Check if each component meets the start/end criteria, else merge with 'do' group
  valid_components = []
  components.each do |component|
    comp_start_activities = dfg.start_activities & component
    comp_end_activities = dfg.end_activities & component
    if comp_start_activities.empty? || comp_end_activities.empty?
      do_group.merge(component)
    else
      valid_components << component
    end
  end

  groups = [do_group] + valid_components

  return groups if groups.size > 1
  nil
end

# Now, run the inductive miner on the DFG
root_node = inductive_miner(dfg)

# Build the process tree
nodes = []
parent_nodes = []

def build_process_tree(node, nodes, parent_nodes)
  nodes << node
  node.children.each do |child|
    parent_node_id = SecureRandom.uuid
    parent_nodes << ParentsNode.new(parent_node_id, node.id, child.id)
    build_process_tree(child, nodes, parent_nodes)
  end
end

build_process_tree(root_node, nodes, parent_nodes)

# Create process_tree_attributes
process_tree_attributes = { 'root' => root_node.id }

# Write the process tree to PTML file
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
nodes.each do |node|
  node_elem = process_tree_elem_out.add_element(node.type)
  node_elem.add_attribute('id', node.id)
  node_elem.add_attribute('name', node.name || '')
end

# Add parentsNode elements to define the relationships
parent_nodes.each do |pn|
  pn_elem = process_tree_elem_out.add_element('parentsNode')
  pn_elem.add_attribute('id', pn.id)
  pn_elem.add_attribute('sourceId', pn.sourceId)
  pn_elem.add_attribute('targetId', pn.targetId)
end

# Write the output document to the output file
begin
  File.open(output_file, 'w') do |file|
    formatter = REXML::Formatters::Pretty.new(2) # Indent by 2 spaces
    formatter.compact = true                     # Remove unnecessary whitespace
    formatter.write(output_doc, file)
  end
  puts "Process tree has been exported to #{output_file}."
rescue => e
  puts "An error occurred during PTML export: #{e.message}"
  exit 1
end