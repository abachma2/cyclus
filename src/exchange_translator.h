#ifndef CYCLUS_SRC_EXCHANGE_TRANSLATOR_H_
#define CYCLUS_SRC_EXCHANGE_TRANSLATOR_H_

#include <sstream>

#include "bid.h"
#include "bid_portfolio.h"
#include "error.h"
#include "exchange_graph.h"
#include "exchange_translation_context.h"
#include "logger.h"
#include "request.h"
#include "request_portfolio.h"
#include "trade.h"

namespace cyclus {

template <class T> class ExchangeContext;
class Trader;

/// @class ExchangeTranslator
///
/// @brief An ExchangeTranslator facilitates translation from a resource
/// specific exchange to a resource-neutral exchange, upon which generic solvers
/// can be applied. Specifically, the ExchangeTranslator constructor takes a
/// resource-specific exchange context and provides a Translate() method to make
/// a resource-neutral ExchangeGraph. State is maintained in the
/// ExchangeTranslator class, mapping Requests and Bids to ExchangeNodes in the
/// ExchangeGraph. Accordingly, the solution to the ExchangeGraph, i.e., it's
/// Matches, can be back-translated to the original Requests and Bids via a
/// BackTranslateSolution() method.
template <class T> class ExchangeTranslator {
 public:
  /// @brief default constructor
  ///
  /// @param ex_ctx the exchance context
  ExchangeTranslator(ExchangeContext<T>* ex_ctx) { ex_ctx_ = ex_ctx; }

  /// @brief translate the ExchangeContext into an ExchangeGraph
  ExchangeGraph::Ptr Translate() {
    ExchangeGraph::Ptr graph(new ExchangeGraph());

    // add each request group
    const std::vector<typename RequestPortfolio<T>::Ptr>& requests =
        ex_ctx_->requests;
    typename std::vector<typename RequestPortfolio<T>::Ptr>::const_iterator
        rp_it;
    for (rp_it = requests.begin(); rp_it != requests.end(); ++rp_it) {
      CapacityConstraint<T> c((*rp_it)->qty(), (*rp_it)->qty_converter());
      (*rp_it)->AddConstraint(c);

      RequestGroup::Ptr rs = TranslateRequestPortfolio(xlation_ctx_, *rp_it);
      graph->AddRequestGroup(rs);
    }

    // add each bid group
    const std::vector<typename BidPortfolio<T>::Ptr>& bidports = ex_ctx_->bids;
    typename std::vector<typename BidPortfolio<T>::Ptr>::const_iterator bp_it;
    for (bp_it = bidports.begin(); bp_it != bidports.end(); ++bp_it) {
      ExchangeNodeGroup::Ptr ns = TranslateBidPortfolio(xlation_ctx_, *bp_it);
      graph->AddSupplyGroup(ns);

      // add each request-bid arc
      const std::set<Bid<T>*>& bids = (*bp_it)->bids();
      typename std::set<Bid<T>*>::const_iterator b_it;
      for (b_it = bids.begin(); b_it != bids.end(); ++b_it) {
        Bid<T>* bid = *b_it;
        Request<T>* req = bid->request();
        AddArc(req, bid, graph);
      }
    }

    return graph;
  }

  /// @brief adds a bid-request arc to a graph, if the preference for the arc is
  /// non-negative
  void AddArc(Request<T>* req, Bid<T>* bid, ExchangeGraph::Ptr graph) {
    double pref = ex_ctx_->trader_prefs.at(req->requester())[req][bid];
    // TODO: make the following check `pref <=0` and remove the `else if` block
    // before release 1.5
    if (pref < 0) {
      CLOG(LEV_DEBUG1) << "Removing arc because of negative preference.";
      return;
    } else if (pref == 0) {
      std::stringstream ss;
      ss << "0-valued preferences have been deprecated. "
         << "Please make preference value positive."
         << "This message will go away in before the next release (1.5).";
      throw ValueError(ss.str());
    }
    // get translated arc
    Arc a = TranslateArc(xlation_ctx_, bid, pref);
    a.unode()->prefs[a] = pref;  // request node is a.unode()
    int n_prefs = a.unode()->prefs.size();

    CLOG(LEV_DEBUG5) << "Updating preference for one of "
                     << req->requester()->manager()->prototype()
                     << "'s trade nodes:";
    CLOG(LEV_DEBUG5) << "   preference: " << a.unode()->prefs[a];

    graph->AddArc(a);
  }

  /// @brief Provide a vector of Trades given a vector of Matches
  void BackTranslateSolution(const std::vector<Match>& matches,
                             std::vector<Trade<T>>& ret) {
    std::vector<Match>::const_iterator m_it;
    CLOG(LEV_DEBUG1) << "Back traslating " << matches.size()
                     << " trade matches.";
    for (m_it = matches.begin(); m_it != matches.end(); ++m_it) {
      ret.push_back(BackTranslateMatch(xlation_ctx_, *m_it));
    }
  }

  const ExchangeTranslationContext<T>& translation_ctx() const {
    return xlation_ctx_;
  }

  ExchangeTranslationContext<T>& translation_ctx() { return xlation_ctx_; }

 private:
  ExchangeContext<T>* ex_ctx_;
  ExchangeTranslationContext<T> xlation_ctx_;
};

/// @brief Adds a request-node mapping
template <class T>
inline void AddRequest(ExchangeTranslationContext<T>& translation_ctx,
                       Request<T>* r, ExchangeNode::Ptr n) {
  translation_ctx.request_to_node[r] = n;
  translation_ctx.node_to_request[n] = r;
}

/// @brief Adds a bid-node mapping
template <class T>
inline void AddBid(ExchangeTranslationContext<T>& translation_ctx, Bid<T>* b,
                   ExchangeNode::Ptr n) {
  translation_ctx.bid_to_node[b] = n;
  translation_ctx.node_to_bid[n] = b;
}

/// @brief translates a request portfolio by adding request nodes and
/// accounting for capacities. Request unit capcities must be added when arcs
/// are known
template <class T>
RequestGroup::Ptr TranslateRequestPortfolio(
    ExchangeTranslationContext<T>& translation_ctx,
    const typename RequestPortfolio<T>::Ptr rp) {
  RequestGroup::Ptr rs(new RequestGroup(rp->qty()));
  CLOG(LEV_DEBUG2) << "Translating request portfolio of size " << rp->qty();

  typename std::vector<Request<T>*>::const_iterator r_it;
  for (r_it = rp->requests().begin(); r_it != rp->requests().end(); ++r_it) {
    Request<T>* r = *r_it;
    ExchangeNode::Ptr n(new ExchangeNode(r->target()->quantity(),
                                         r->exclusive(),
                                         r->commodity(),
                                         r->requester()->manager()->id()));
    rs->AddExchangeNode(n);

    AddRequest(translation_ctx, *r_it, n);
  }

  CLOG(LEV_DEBUG4) << "adding " << rp->constraints().size()
                   << " request capacities";
  typename std::set<CapacityConstraint<T>>::const_iterator c_it;
  for (c_it = rp->constraints().begin(); c_it != rp->constraints().end();
       ++c_it) {
    rs->AddCapacity(c_it->capacity());
  }

  return rs;
}

/// @brief translates a bid portfolio by adding bid nodes and accounting
/// for capacities. Bid unit capcities must be added when arcs are known
template <class T>
ExchangeNodeGroup::Ptr TranslateBidPortfolio(
    ExchangeTranslationContext<T>& translation_ctx,
    const typename BidPortfolio<T>::Ptr bp) {
  ExchangeNodeGroup::Ptr bs(new ExchangeNodeGroup());

  std::map<typename T::Ptr, std::vector<ExchangeNode::Ptr>> excl_bid_grps;

  typename std::set<Bid<T>*>::const_iterator b_it;
  for (b_it = bp->bids().begin(); b_it != bp->bids().end(); ++b_it) {
    Bid<T>* b = *b_it;
    ExchangeNode::Ptr n(new ExchangeNode(b->offer()->quantity(),
                                         b->exclusive(),
                                         b->request()->commodity(),
                                         b->bidder()->manager()->id()));
    bs->AddExchangeNode(n);
    AddBid(translation_ctx, *b_it, n);
    if (b->exclusive()) {
      excl_bid_grps[b->offer()].push_back(n);
    }
  }

  typename std::map<typename T::Ptr, std::vector<ExchangeNode::Ptr>>::iterator
      m_it;
  for (m_it = excl_bid_grps.begin(); m_it != excl_bid_grps.end(); ++m_it) {
    bs->AddExclGroup(m_it->second);
  }

  CLOG(LEV_DEBUG4) << "adding " << bp->constraints().size()
                   << " bid capacities";

  typename std::set<CapacityConstraint<T>>::const_iterator c_it;
  for (c_it = bp->constraints().begin(); c_it != bp->constraints().end();
       ++c_it) {
    bs->AddCapacity(c_it->capacity());
  }

  return bs;
}

/// @brief translates an arc given a bid and subsequent data, and also
/// updates the unit capacities for the associated nodes on the arc
template <class T>
Arc TranslateArc(const ExchangeTranslationContext<T>& translation_ctx,
                 Bid<T>* bid) {
  return TranslateArc<T>(translation_ctx, bid, 1);
}

template <class T>
Arc TranslateArc(const ExchangeTranslationContext<T>& translation_ctx,
                 Bid<T>* bid, double pref) {
  Request<T>* req = bid->request();
  ExchangeNode::Ptr unode = translation_ctx.request_to_node.at(req);
  ExchangeNode::Ptr vnode = translation_ctx.bid_to_node.at(bid);
  Arc arc(unode, vnode);
  arc.pref(pref);

  typename T::Ptr offer = bid->offer();
  typename BidPortfolio<T>::Ptr bp = bid->portfolio();
  typename RequestPortfolio<T>::Ptr rp = req->portfolio();

  // bid is v
  TranslateCapacities(offer, bp->constraints(), vnode, arc, translation_ctx);
  // req is u
  TranslateCapacities(offer, rp->constraints(), unode, arc, translation_ctx);

  return arc;
}

/// @brief simple translation from a Match to a Trade, given internal state
template <class T>
Trade<T> BackTranslateMatch(
    const ExchangeTranslationContext<T>& translation_ctx, const Match& match) {
  ExchangeNode::Ptr req_node = match.first.unode();
  ExchangeNode::Ptr bid_node = match.first.vnode();

  Trade<T> t;
  t.request = translation_ctx.node_to_request.at(req_node);
  t.bid = translation_ctx.node_to_bid.at(bid_node);
  t.amt = match.second;
  return t;
}

/// @brief updates a node's unit capacities given, a target resource and
/// constraints
template <typename T>
void TranslateCapacities(typename T::Ptr offer,
                         const typename std::set<CapacityConstraint<T>>& constr,
                         ExchangeNode::Ptr n,
                         const Arc& a,
                         const ExchangeTranslationContext<T>& ctx) {
  typename std::set<CapacityConstraint<T>>::const_iterator it;
  for (it = constr.begin(); it != constr.end(); ++it) {
    CLOG(cyclus::LEV_DEBUG1)
        << "Additing unit capacity: "
        << it->convert(offer, &a, &ctx) / offer->quantity();
    n->unit_capacities[a].push_back(it->convert(offer, &a, &ctx) /
                                    offer->quantity());
  }
}

}  // namespace cyclus

#endif  // CYCLUS_SRC_EXCHANGE_TRANSLATOR_H_
