/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef nsContainerFrame_h___
#define nsContainerFrame_h___

#include "nsSplittableFrame.h"

/**
 * Implementation of a container frame. Supports being used a pseudo-
 * frame (a frame that maps the same content as its parent).
 *
 * Container frame iterates its child content and for each content object creates
 * one or more child frames. There are three member variables (class invariants)
 * used to manage this mapping:
 * <dl>
 * <dt>mFirstContentOffset
 *   <dd>the content offset of the first piece of content mapped by this container.
 * <dt>mLastContentOffset
 *   <dd>the content offset of the last piece of content mapped by this container
 * <dt>mLastContentIsComplete
 *   <dd>a boolean indicating whether the last piece of content mapped by this
 *       container is complete, or whether its frame needs to be continued.
 * </dl>
 *
 * Here are the rules governing the state of the class invariants. The debug member
 * functions PreReflowCheck() and PostReflowCheck() validate these rules. For the
 * purposes of this discussion an <i>empty</i> frame is defined as a container
 * frame with a null child-list, i.e. its mFirstChild is nsnull.
 *
 * <h3>Non-Empty Frames</h3>
 * For a container frame that is not an empty frame the following must be true:
 * <ul>
 * <li>if the first child frame is a pseudo-frame then mFirstContentOffset must be
 *     equal to the first child's mFirstContentOffset; otherwise, mFirstContentOffset
 *     must be equal to the first child frame's index in parent
 * <li>if the last child frame is a pseudo-frame then mLastContentOffset must be
 *     equal to the last child's mLastContentOffset; otherwise, mLastContentOffset
 *     must be equal to the last child frame's index in parent
 * <li>if the last child is a pseudo-frame then mLastContentIsComplete should be
 *     equal to that last child's mLastContentIsComplete; otherwise, if the last
 *     child frame has a next-in-flow then mLastContentIsComplete must be PR_TRUE
 * </ul>
 *
 * <h3>Empty Frames</h3>
 * For an empty container frame the following must be true:
 * <ul>
 * <li>mFirstContentOffset must be equal to the NextContentOffset() of the first
 *     non-empty prev-in-flow
 * <li>mChildCount must be 0
 * </ul>
 *
 * The value of mLastContentOffset and mLastContentIsComplete are <i>undefined</i>
 * for an empty frame.
 *
 * <h3>Next-In-Flow List</h3>
 * The rule is that if a container frame is not complete then the mFirstContentOffset,
 * mLastContentOffset, and mLastContentIsComplete of its next-in-flow frames must
 * <b>always</b> correct. This means that whenever you push/pull child frames from a
 * next-in-flow you <b>must</b> update that next-in-flow's state so that it is
 * consistent with the aforementioned rules for empty and non-empty frames.
 *
 * <h3>Pulling-Up Frames</h3>
 * In the processing of pulling-up child frames from a next-in-flow it's possible
 * to pull-up all the child frames from a next-in-flow thereby leaving the next-in-flow
 * empty. There are two cases to consider:
 * <ol>
 * <li>All of the child frames from all of the next-in-flow have been pulled-up.
 *     This means that all the next-in-flow frames are empty, the container being
 *     reflowed is complete, and the next-in-flows will be deleted.
 *
 *     In this case the next-in-flows' mFirstContentOffset is also undefined.
 * <li>Not all the child frames from all the next-in-flows have been pulled-up.
 *     This means the next-in-flow consists of one or more empty frames followed
 *     by one or more non-empty frames. Note that all the empty frames must be
 *     consecutive. This means it is illegal to have an empty frame followed by
 *     a non-empty frame, followed by an empty frame, followed by a non-empty frame.
 * </ol>
 *
 * <h3>Pseudo-Frames</h3>
 * As stated above, when pulling/pushing child frames from/to a next-in-flow the
 * state of the next-in-flow must be updated.
 *
 * If the next-in-flow is a pseudo-frame then not only does the next-in-flow's state
 * need to be updated, but the state of its geometric parent must be updated as well.
 * See member function PropagateContentOffsets() for details.
 *
 * This rule is applied recursively, so if the next-in-flow's geometric parent is
 * also a pseudo-frame then its geometric parent must be updated.
 */
class nsContainerFrame : public nsSplittableFrame
{
public:
  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler) const;

  NS_IMETHOD  Init(nsIPresContext& aPresContext, nsIFrame* aChildList);

  NS_IMETHOD  DeleteFrame(nsIPresContext& aPresContext);

  NS_IMETHOD DidReflow(nsIPresContext& aPresContext,
                       nsDidReflowStatus aStatus);

  // Painting
  NS_IMETHOD Paint(nsIPresContext&      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect);

  /**
   * Pass through the event to the correct child frame.
   * Return PR_TRUE if the event is consumed.
   */
  NS_IMETHOD HandleEvent(nsIPresContext& aPresContext,
                         nsGUIEvent*     aEvent,
                         nsEventStatus&  aEventStatus);

  NS_IMETHOD GetCursorAndContentAt(nsIPresContext& aPresContext,
                         const nsPoint&  aPoint,
                         nsIFrame**      aFrame,
                         nsIContent**    aContent,
                         PRInt32&        aCursor);

  // Child frame enumeration.
  // ChildAt() retruns null if the index is not in the range 0 .. ChildCount() - 1.
  // IndexOf() returns -1 if the frame is not in the child list.
  NS_IMETHOD  FirstChild(nsIFrame*& aFirstChild) const;

  // Returns true if this frame is being used a pseudo frame
  // XXX deprecated
  PRBool      IsPseudoFrame() const;

  // Debugging
  NS_IMETHOD  List(FILE* out = stdout, PRInt32 aIndent = 0, nsIListFilter *aFilter = nsnull) const;
  NS_IMETHOD  ListTag(FILE* out = stdout) const;
  NS_IMETHOD  VerifyTree() const;

  /**
   * Return the number of children in the sibling list, starting at aChild.
   * Returns zero if aChild is nsnull.
   */
  static PRInt32 LengthOf(nsIFrame* aChild);

  /**
   * Return the last frame in the sibling list.
   * Returns nsnullif aChild is nsnull.
   */
  static nsIFrame* LastFrame(nsIFrame* aChild);

  /**
   * Returns the frame at the specified index relative to aFrame
   */
  static nsIFrame* FrameAt(nsIFrame* aFrame, PRInt32 aIndex);

  // XXX needs to be virtual so that nsBlockFrame can override it
  virtual PRBool DeleteChildsNextInFlow(nsIPresContext& aPresContext,
                                        nsIFrame* aChild);

protected:
  // Constructor. Takes as arguments the content object, the index in parent,
  // and the Frame for the content parent
  nsContainerFrame(nsIContent* aContent, nsIFrame* aParent);

  virtual ~nsContainerFrame();

  void SizeOfWithoutThis(nsISizeOfHandler* aHandler) const;

  /**
   * Prepare a continuation frame of this frame for reflow. Appends
   * it to the flow, sets its content offsets, mLastContentIsComplete,
   * and style context.  Subclasses should invoke this method after
   * construction of a continuing frame.
   */
  void PrepareContinuingFrame(nsIPresContext&   aPresContext,
                              nsIFrame*         aParent,
                              nsIStyleContext*  aStyleContext,
                              nsContainerFrame* aContFrame);


  virtual void  PaintChildren(nsIPresContext&      aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              const nsRect&        aDirtyRect);

  /**
   * Reflow a child frame and return the status of the reflow. If the child
   * is complete and it has next-in-flows, then delete the next-in-flows.
   */
  nsReflowStatus ReflowChild(nsIFrame*            aKidFrame,
                             nsIPresContext*      aPresContext,
                             nsReflowMetrics&     aDesiredSize,
                             const nsReflowState& aReflowState);

 /**
  * Moves any frames on both the prev-in-flow's overflow list and the receiver's
  * overflow to the receiver's child list.
  *
  * Resets the overlist pointers to nsnull, and updates the receiver's child
  * count and content mapping.
  *
  * @return  PR_TRUE if any frames were moved and PR_FALSE otherwise
  */
  PRBool MoveOverflowToChildList();

 /**
  * Push aFromChild and its next siblings to the next-in-flow. Change the
  * geometric parent of each frame that's pushed. If there is no next-in-flow
  * the frames are placed on the overflow list (and the geometric parent is
  * left unchanged).
  *
  * Updates the next-in-flow's child count. Does <b>not</b> update the
  * pusher's child count.
  *
  * @param   aFromChild the first child frame to push. It is disconnected from
  *            aPrevSibling
  * @param   aPrevSibling aFromChild's previous sibling. Must not be null. It's
  *            an error to push a parent's first child frame
  */
  void PushChildren(nsIFrame* aFromChild, nsIFrame* aPrevSibling);

  /**
   * Append child list starting at aChild to this frame's child list. Used for
   * processing of the overflow list.
   *
   * Updates this frame's child count and content mapping.
   *
   * @param   aChild the beginning of the child list
   * @param   aSetParent if true each child's geometric (and content parent if
   *            they're the same) parent is set to this frame.
   */
  void AppendChildren(nsIFrame* aChild, PRBool aSetParent = PR_TRUE);

  virtual void WillDeleteNextInFlowFrame(nsIFrame* aNextInFlow);

#ifdef NS_DEBUG
  /**
   * Returns PR_TRUE if aChild is a child of this frame.
   */
  PRBool IsChild(const nsIFrame* aChild) const;

  void DumpTree() const;
#endif

  nsIFrame*   mFirstChild;
  PRInt32     mChildCount;
  nsIFrame*   mOverflowList;
};

#endif /* nsContainerFrame_h___ */
