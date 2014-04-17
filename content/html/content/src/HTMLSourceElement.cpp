/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/HTMLSourceElement.h"
#include "mozilla/dom/HTMLSourceElementBinding.h"

#include "nsIMediaList.h"
#include "nsCSSParser.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Source)

namespace mozilla {
namespace dom {

HTMLSourceElement::HTMLSourceElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

HTMLSourceElement::~HTMLSourceElement()
{
}

NS_IMPL_ISUPPORTS_INHERITED(HTMLSourceElement, nsGenericHTMLElement,
                            nsIDOMHTMLSourceElement)

NS_IMPL_ELEMENT_CLONE(HTMLSourceElement)

NS_IMPL_URI_ATTR(HTMLSourceElement, Src, src)
NS_IMPL_STRING_ATTR(HTMLSourceElement, Type, type)
NS_IMPL_STRING_ATTR(HTMLSourceElement, Srcset, srcset)
NS_IMPL_STRING_ATTR(HTMLSourceElement, Sizes, sizes)
NS_IMPL_STRING_ATTR(HTMLSourceElement, Media, media)

bool
HTMLSourceElement::MatchesCurrentMedia()
{
  if (mMediaList) {
    nsIPresShell* presShell = OwnerDoc()->GetShell();
    nsPresContext* pctx = presShell ? presShell->GetPresContext() : nullptr;
    return pctx && mMediaList->Matches(pctx, nullptr);
  }

  // No media specified
  return true;
}

nsresult
HTMLSourceElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify)
{
  if (aName == nsGkAtoms::media) {
    mMediaList = nullptr;
    if (aValue) {
      nsString mediaStr = aValue->GetStringValue();
      if (!mediaStr.IsEmpty()) {
        nsCSSParser cssParser;
        mMediaList = new nsMediaList();
        cssParser.ParseMediaList(mediaStr, nullptr, 0, mMediaList, false);
      }
    }
  }

  return nsGenericHTMLElement::AfterSetAttr(aNameSpaceID, aName,
                                            aValue, aNotify);
}

void
HTMLSourceElement::GetItemValueText(nsAString& aValue)
{
  GetSrc(aValue);
}

void
HTMLSourceElement::SetItemValueText(const nsAString& aValue)
{
  SetSrc(aValue);
}

nsresult
HTMLSourceElement::BindToTree(nsIDocument *aDocument,
                              nsIContent *aParent,
                              nsIContent *aBindingParent,
                              bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument,
                                                 aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aParent || !aParent->IsNodeOfType(nsINode::eMEDIA))
    return NS_OK;

  HTMLMediaElement* media = static_cast<HTMLMediaElement*>(aParent);
  media->NotifyAddedSource();

  return NS_OK;
}

JSObject*
HTMLSourceElement::WrapNode(JSContext* aCx)
{
  return HTMLSourceElementBinding::Wrap(aCx, this);
}

} // namespace dom
} // namespace mozilla
