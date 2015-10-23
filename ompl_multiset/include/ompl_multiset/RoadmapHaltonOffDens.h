/* File: RoadmapHalton.h
 * Author: Chris Dellin <cdellin@gmail.com>
 * Copyright: 2015 Carnegie Mellon University
 * License: BSD
 */

namespace ompl_multiset
{

// for now this is an r-disk prm,
// uniform milestone sampling with given seed,
// uses the space's default sampler
//template <class Graph, class VertexIndexMap, class EdgeIndexMap//,
   //class StateMap, class BatchMap, class IsShadowMap, class DistanceMap
//   >
template <class RoadmapSpec>
class RoadmapHaltonOffDens : public RoadmapSpec
{
   typedef typename RoadmapSpec::BaseGraph Graph;
   typedef typename RoadmapSpec::BaseVState VState;
   typedef typename RoadmapSpec::BaseEDistance EDistance;
   typedef typename RoadmapSpec::BaseVBatch VBatch;
   typedef typename RoadmapSpec::BaseEBatch EBatch;
   typedef typename RoadmapSpec::BaseVShadow VShadow;
   typedef typename RoadmapSpec::BaseNN NN;

   typedef boost::graph_traits<Graph> GraphTypes;
   typedef typename GraphTypes::vertex_descriptor Vertex;
   typedef typename GraphTypes::edge_descriptor Edge;
   typedef typename boost::property_traits<VState>::value_type::element_type StateCon;
   
public:
   // input parameters
   const unsigned int n_perbatch;
   const double radius_firstbatch;
   const unsigned int seed;
   
   RoadmapHaltonOffDens(
      const ompl::base::StateSpacePtr space,
      unsigned int n_perbatch, double radius_firstbatch, unsigned int seed):
      RoadmapSpec(space,0),
      n_perbatch(n_perbatch), radius_firstbatch(radius_firstbatch), seed(seed),
      dim(0),
      bounds(0),
      offset_state(space),
      num_batches_generated(0),
      vertices_generated(0),
      edges_generated(0)
   {
      // check that we're in a real vector state space
      if (space->getType() != ompl::base::STATE_SPACE_REAL_VECTOR)
         throw std::runtime_error("RoadmapHaltonDens only supports rel vector state spaces!");
      dim = space->getDimension();
      if (0 == ompl_multiset::util::get_prime(dim-1))
         throw std::runtime_error("not enough primes hardcoded!");
      bounds = space->as<ompl::base::RealVectorStateSpace>()->getBounds();
      
      ompl::base::StateSamplerPtr sampler(space->allocStateSampler());
      ompl_multiset::SamplerGenMonkeyPatch(sampler) = boost::mt19937(seed);
      sampler->sampleUniform(offset_state.get());
      offset_values = offset_state.get()->as<ompl::base::RealVectorStateSpace::StateType>()->values;
   }
   ~RoadmapHaltonOffDens() {}
   
   std::size_t get_num_batches_generated()
   {
      return num_batches_generated;
   }
   
   double root_radius(std::size_t i_batch)
   {
      return radius_firstbatch
         * pow(1./(i_batch+1.), 1./dim);
   }
   
   void generate(
      Graph & g,
      NN & nn,
      VState state_map,
      EDistance distance_map,
      VBatch vertex_batch_map,
      EBatch edge_batch_map,
      VShadow is_shadow_map)
   {
      // compute radius
      double radius = root_radius(num_batches_generated);
      while (num_vertices(g) < (num_batches_generated+1) * n_perbatch)
      {
         Vertex v_new = add_vertex(g);
         
         put(vertex_batch_map, v_new, num_batches_generated);
         put(is_shadow_map, v_new, false);
         
         // allocate a new state for this vertex
         get(state_map, v_new).reset(new StateCon(this->space.get()));
         ompl::base::State * v_state = get(state_map, v_new)->state;
         double * values = v_state->as<ompl::base::RealVectorStateSpace::StateType>()->values;
         for (unsigned int ui=0; ui<dim; ui++)
         {
            double value = offset_values[ui];
            value += (bounds.high[ui] - bounds.low[ui])
               * ompl_multiset::util::halton(ompl_multiset::util::get_prime(ui), vertices_generated);
            if (bounds.high[ui] < value)
               value -= (bounds.high[ui] - bounds.low[ui]);
            values[ui] = value;
         }
                  
         // allocate new undirected edges
         std::vector< std::pair<Vertex,double> > vs_near;
         nn.nearestR(v_new, radius, vs_near);
         for (unsigned int ui=0; ui<vs_near.size(); ui++)
         {
            Edge e = add_edge(v_new, vs_near[ui].first, g).first;
            put(distance_map, e, vs_near[ui].second);
            put(edge_batch_map, e, num_batches_generated);
            edges_generated++;
         }
         
         vertices_generated++;
      }
      num_batches_generated++;
   }
   
   void serialize()
   {
   }
   
   void deserialize()
   {
   }
   
private:
   // from space
   unsigned int dim;
   ompl::base::RealVectorBounds bounds;
   ompl::base::ScopedState<ompl::base::RealVectorStateSpace> offset_state;
   double * offset_values;
   // progress
   std::size_t num_batches_generated;
   std::size_t vertices_generated;
   std::size_t edges_generated;
};

} // namespace ompl_multiset