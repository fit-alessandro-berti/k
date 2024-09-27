# This script implements the Alpha Miner process discovery algorithm in Ruby.
# It accepts two arguments: the input XES event log file, and the output PNML file.
# Usage: ruby alpha_miner.rb input_file.xes output_file.pnml

require 'set'
require_relative 'rb_xes'
require_relative 'rb_pnml'
class AlphaMiner
  def initialize(traces)
    @traces = traces
    @activities = Set.new
    @direct_successions = Set.new
    @causalities = Set.new
    @parallel = Set.new
    @choices = Set.new
    @footprint = {}
  end

  def discover_petri_net
    # Step 1: Extract activities and direct succession relations
    extract_activities_and_relations

    # Step 2: Build relations
    build_relations

    # Step 3: Identify places (Y_W)
    y_set = identify_places

    # Step 4: Identify transitions (T_I and T_O)
    t_i = identify_initial_transitions
    t_o = identify_final_transitions

    # Step 5: Build the Petri net
    petri_net = build_petri_net(y_set, t_i, t_o)
    petri_net
  end

  private

  def extract_activities_and_relations
    @traces.each do |trace|
      # Collect activities
      trace.each do |event|
        @activities.add(event)
      end

      # Collect direct successions
      (0...trace.size - 1).each do |i|
        @direct_successions.add([trace[i], trace[i + 1]])
      end
    end
  end

  def build_relations
    # Initialize footprint matrix
    @activities.each do |a|
      @footprint[a] = {}
      @activities.each do |b|
        @footprint[a][b] = nil
      end
    end

    # Build direct succession relations
    @direct_successions.each do |(a, b)|
      @footprint[a][b] = '>'
    end

    # Build causality, parallel, and choice relations
    @activities.each do |a|
      @activities.each do |b|
        next if a == b

        if @footprint[a][b] == '>' && @footprint[b][a] != '>'
          @causalities.add([a, b])
          @footprint[a][b] = '->'
          @footprint[b][a] = '<-'
        elsif @footprint[a][b] == '>' && @footprint[b][a] == '>'
          @parallel.add([a, b])
          @footprint[a][b] = '||'
          @footprint[b][a] = '||'
        elsif @footprint[a][b] != '>' && @footprint[b][a] != '>'
          @choices.add([a, b])
          @footprint[a][b] = '#'
          @footprint[b][a] = '#'
        end
      end
    end
  end

  def identify_places
    y_set = []

    # Generate all possible subsets of activities
    subsets_a = all_non_empty_subsets(@activities.to_a)
    subsets_b = subsets_a.dup

    subsets_a.each do |a|
      subsets_b.each do |b|
        next if a & b != []

        # Check conditions:
        # Neither A x A nor B x B contains any '>' relations
        next if contains_direct_successions?(a)
        next if contains_direct_successions?(b)

        # A x B is subset of '->'
        if all_causalities?(a, b)
          y_set << [Set.new(a), Set.new(b)]
        end
      end
    end

    # Select maximal sets
    maximal_y_set = []
    y_set.each do |pair|
      is_maximal = true
      y_set.each do |other_pair|
        next if pair == other_pair
        if pair[0].subset?(other_pair[0]) && pair[1].subset?(other_pair[1])
          if pair[0] != other_pair[0] || pair[1] != other_pair[1]
            is_maximal = false
            break
          end
        end
      end
      maximal_y_set << pair if is_maximal
    end

    maximal_y_set
  end

  def all_non_empty_subsets(array)
    subsets = []
    (1..array.size).each do |n|
      subsets += array.combination(n).to_a
    end
    subsets
  end

  def contains_direct_successions?(activities)
    activities.each do |a|
      activities.each do |b|
        next if a == b
        return true if @direct_successions.include?([a, b])
      end
    end
    false
  end

  def all_causalities?(activities_a, activities_b)
    activities_a.each do |a|
      activities_b.each do |b|
        return false unless @causalities.include?([a, b])
      end
    end
    true
  end

  def identify_initial_transitions
    t_i = Set.new
    @traces.each do |trace|
      t_i.add(trace.first)
    end
    t_i
  end

  def identify_final_transitions
    t_o = Set.new
    @traces.each do |trace|
      t_o.add(trace.last)
    end
    t_o
  end

  def build_petri_net(y_set, t_i, t_o)
    petri_net = PetriNet.new

    # Add transitions
    @activities.each do |activity|
      transition = Transition.new("t_#{activity}", activity)
      petri_net.add_transition(transition)
    end

    # Add places
    place_id_counter = 1
    place_map = {}

    y_set.each do |(a_set, b_set)|
      place_id = "p_#{place_id_counter}"
      place = Place.new(place_id, "")
      petri_net.add_place(place)
      place_map[[a_set, b_set]] = place_id
      place_id_counter += 1
    end

    # Add input place i_W
    input_place = Place.new("p_input", "i")
    petri_net.add_place(input_place)
    petri_net.initial_markings[input_place.id] = 1

    # Add output place o_W
    output_place = Place.new("p_output", "o")
    petri_net.add_place(output_place)

    # Add arcs for Y_W
    y_set.each do |(a_set, b_set)|
      place_id = place_map[[a_set, b_set]]

      a_set.each do |a|
        petri_net.add_arc(Arc.new("arc_t_#{a}_p_#{place_id}", "t_#{a}", place_id))
      end

      b_set.each do |b|
        petri_net.add_arc(Arc.new("arc_p_#{place_id}_t_#{b}", place_id, "t_#{b}"))
      end
    end

    # Add arcs from input place to initial transitions
    t_i.each do |t|
      petri_net.add_arc(Arc.new("arc_p_input_t_#{t}", "p_input", "t_#{t}"))
    end

    # Add arcs from final transitions to output place
    t_o.each do |t|
      petri_net.add_arc(Arc.new("arc_t_#{t}_p_output", "t_#{t}", "p_output"))
    end

    petri_net
  end
end

# Main program
if __FILE__ == $0
  if ARGV.length != 2
    puts "Usage: ruby alpha_miner.rb input_log.xes output_petri_net.pnml"
    exit
  end

  xes_file = ARGV[0]
  pnml_file = ARGV[1]

  # Import the XES event log
  xes_importer = XESImporterExporter.new
  xes_importer.import(xes_file)
  traces = xes_importer.traces

  # Apply Alpha Miner
  alpha_miner = AlphaMiner.new(traces)
  petri_net = alpha_miner.discover_petri_net

  # Export the Petri net to PNML
  write_pnml(petri_net, pnml_file)

  puts "Petri net has been successfully exported to #{pnml_file}"
end
