/*
  The contents of this file are dedicated by all of its authors, including

    Michael S. Gashler,
    Eric Moyer,
    anonymous contributors,

  to the public domain (http://creativecommons.org/publicdomain/zero/1.0/).

  Note that some moral obligations still exist in the absence of legal ones.
  For example, it would still be dishonest to deliberately misrepresent the
  origin of a work. Although we impose no legal requirements to obtain a
  license, it is beseeming for those who build on the works of others to
  give back useful improvements, or find a way to pay it forward. If
  you would like to cite us, a published paper about Waffles can be found
  at http://jmlr.org/papers/volume12/gashler11a/gashler11a.pdf. If you find
  our code to be useful, the Waffles team would love to hear how you use it.
*/

#ifndef __GTRANSFORM_H__
#define __GTRANSFORM_H__

#include "GLearner.h"

namespace GClasses {

class GActivationFunction;

/// This is the base class of algorithms that transform data without supervision
class GTransform
{
public:
	GTransform();
	GTransform(GDomNode* pNode, GLearnerLoader& ll);
	virtual ~GTransform();

	/// Applies the transformation to pIn and returns the results.
	virtual GMatrix* reduce(const GMatrix& in) = 0;

protected:
	/// Child classes should use this in their implementation of serialize
	virtual GDomNode* baseDomNode(GDom* pDoc, const char* szClassName) const;
};




/// This is the base class of algorithms that can transform data
/// one row at a time without supervision.
class GIncrementalTransform : public GTransform
{
private:
	GRelation* m_pRelationBefore;
	GRelation* m_pRelationAfter;
	GVec m_innerBuf;

public:
	GIncrementalTransform() : GTransform(), m_pRelationBefore(NULL), m_pRelationAfter(NULL) {}
	GIncrementalTransform(GDomNode* pNode, GLearnerLoader& ll);
	virtual ~GIncrementalTransform();

#ifndef MIN_PREDICT
	/// Performs unit tests for this class. Throws an exception if there is a failure.
	static void test();

	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const = 0;
#endif // MIN_PREDICT

	/// Trains the transform on the data in pData. (This method may be a no-op
	/// for transformations that always behave in the same manner.)
	void train(const GMatrix& data);

	/// "Trains" the transform without any data.
	///
	/// This method is called to initialize the transform
	/// when it is used with an incremental learner.
	/// Transforms that cannot be trained without available
	/// data may throw an exception in this method.
	void train(const GRelation& pRelation);

	/// Sets the before relation. Takes ownership of pRel.
	void setBefore(GRelation* pRel);

	/// Sets the after relation. Takes ownership of pRel.
	void setAfter(GRelation* pRel);

	/// Returns a relation object describing the data before it is
	/// transformed
	///
	/// train must be called before this method is used
	const GRelation& before() const { return *m_pRelationBefore; }

	/// Returns a relation object describing the data after it is
	/// transformed
	///
	/// train must be called before this method is used
	const GRelation& after() const { return *m_pRelationAfter; }

	/// pIn is the source row. pOut is a buffer that will hold the
	/// transformed row.  train must be called before this method
	/// is used
	virtual void transform(const GVec& in, GVec& out) = 0;

	/// This calls train, then calls transformBatch, and returns the result.
	virtual GMatrix* reduce(const GMatrix& in);

	/// This assumes that train has already been called, and
	/// transforms all the rows in in returning the resulting
	/// matrix.  The caller is responsible for deleting the new
	/// matrix.
	virtual GMatrix* transformBatch(const GMatrix& in);

	/// Returns a buffer of sufficient size to store an inner
	/// (transformed) vector. The caller should not to delete
	/// the buffer. The same buffer will be returned each time.
	GVec& innerBuf();

	/// pIn is a previously transformed row, and pOut is a buffer that will hold the untransformed row.
	/// train must be called before this method is used.
	/// This method may throw an exception if this transformation cannot be undone or approximately undone.
	virtual void untransform(const GVec& in, GVec& out) = 0;

#ifndef MIN_PREDICT
	/// Similar to untransform, except it produces a distribution instead of just a vector.
	/// This method may not be implemented in all classes, so it may throw an exception.
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut) = 0;
#endif // MIN_PREDICT

	/// This assumes train was previously called, and untransforms all the rows in pIn and returns the results.
	virtual GMatrix* untransformBatch(const GMatrix& in);

protected:
	/// Child classes should use this in their implementation of serialize
	virtual GDomNode* baseDomNode(GDom* pDoc, const char* szClassName) const;

	/// This method implements the functionality called by train.
	/// The data passed in may be used to guide training.
	/// This method returns a smart-pointer to a relation the represents
	/// the form that the data will take after it is transformed.
	virtual GRelation* trainInner(const GMatrix& data) = 0;

	/// This method implements the functionality called by train.
	/// This method is called to initialize the transform
	/// when it is used with an incremental learner.
	/// Transforms that cannot be trained without available
	/// data may throw an exception in this method.
	/// The relation passed in represents the form that the input
	/// data will have.
	/// This method returns a smart-pointer to a relation the represents
	/// the form that the data will take after it is transformed.
	virtual GRelation* trainInner(const GRelation& relation) = 0;
};




/// This wraps two two-way-incremental-transoforms to form a single combination transform
class GIncrementalTransformChainer : public GIncrementalTransform
{
protected:
	GIncrementalTransform* m_pFirst;
	GIncrementalTransform* m_pSecond;

public:
	/// General-purpose constructor
	GIncrementalTransformChainer(GIncrementalTransform* pFirst, GIncrementalTransform* pSecond);

	/// Deserializing constructor
	GIncrementalTransformChainer(GDomNode* pNode, GLearnerLoader& ll);
	virtual ~GIncrementalTransformChainer();

#ifndef MIN_PREDICT
	/// See the comment for GIncrementalTransform::serialize
	virtual GDomNode* serialize(GDom* pDoc) const;
#endif // MIN_PREDICT

	/// See the comment for GIncrementalTransform::train
	virtual void transform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransform
	virtual void untransform(const GVec& in, GVec& out);

#ifndef MIN_PREDICT
	/// See the comment for GIncrementalTransform::untransformToDistribution
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut);
#endif // MIN_PREDICT

protected:
	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GRelation& relation);
};






/// Principal Component Analysis. (Computes the principal components about
/// the mean of the data when you call train. The transformed (reduced-dimensional)
/// data will have a mean about the origin.)
class GPCA : public GIncrementalTransform
{
protected:
	size_t m_targetDims;
	GMatrix* m_pBasisVectors;
	GMatrix* m_pCentroid;
	GVec m_eigVals;
	bool m_aboutOrigin;
	GRand m_rand;

public:
	GPCA(size_t targetDims);

	/// Load from a DOM.
	GPCA(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GPCA();

#ifndef MIN_PREDICT
	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const;
#endif // MIN_PREDICT

	/// Specify to compute the eigenvalues during training. This
	/// method must be called before train is called.
	void computeEigVals();

	/// Specify to compute the principal components about the origin (instead
	/// of computing them about the mean).
	void aboutOrigin() { m_aboutOrigin = true; }

	/// Returns the eigenvalues. Returns NULL if computeEigVals was not called.
	GVec& eigVals() { return m_eigVals; }

	/// Returns the number of principal components that it will find.
	size_t targetDims() { return m_targetDims; }

	/// Returns the centroid about which the principal components were calculated.
	GVec& centroid() { return m_pCentroid->row(0); }

	/// Returns the matrix of basis vectors. Row 0 is the first principal component,
	/// row 1 is the second principal component, and so forth.
	GMatrix* basis() { return m_pBasisVectors; }

	/// Returns a dataset where the first row is the centroid, and the remaining
	/// rows are the principal component vectors in order of decreasing eigenvalue.
	GMatrix* components() { return m_pBasisVectors; }

	/// See the comment for GIncrementalTransform::transform.
	/// Projects the specified point into fewer dimensions.
	virtual void transform(const GVec& in, GVec& out);

	/// Computes a (lossy) high-dimensional point that corresponds with the
	/// specified low-dimensional coordinates.
	virtual void untransform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransformToDistribution
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut);

	/// Returns a reference to the pseudo-random number generator used by this object.
	GRand& rand() { return m_rand; }
protected:
	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// Throws an exception (because this transform cannot be trained without data)
	virtual GRelation* trainInner(const GRelation& relation);
};


/// Principle Component Analysis without the projection. It only rotates
/// axes to align with the first few principal components.
class GPCARotateOnly
{
public:
	/// This rotates the data to align the first nComponents axes with the same
	/// number of principle components.
	static GMatrix* transform(size_t nDims, size_t nOutputs, const GMatrix* pData, size_t nComponents, GRand* pRand);

#ifndef MIN_PREDICT
	/// Performs unit tests for this class. Throws an exception if there is a failure.
	static void test();
#endif // MIN_PREDICT
};


/// Just generates Gaussian noise
class GNoiseGenerator : public GIncrementalTransform
{
protected:
	GRand m_rand;
	double m_mean, m_deviation;

public:
	GNoiseGenerator();

	/// Load from a DOM.
	GNoiseGenerator(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GNoiseGenerator();

	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const;

	/// See the comment for GIncrementalTransform::transform
	virtual void transform(const GVec& in, GVec& out);

	/// Sets the mean and deviation of the noise
	void setMeanAndDeviation(double m, double d) { m_mean = m; m_deviation = d; }

	/// Throws an exception (because this transform cannot be reversed).
	virtual void untransform(const GVec& in, GVec& out)
	{ throw Ex("This transformation cannot be reversed"); }

	/// Throws an exception (because this transform cannot be undone).
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut)
	{ throw Ex("This transformation cannot be reversed"); }

	/// Returns a reference to the rand object used to generate noise.
	GRand& rand() { return m_rand; }

protected:
	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GRelation& relation);
};



/// Generates data by computing the product of each pair of
/// attributes. This is useful for augmenting data.
class GPairProduct : public GIncrementalTransform
{
protected:
	size_t m_maxDims;

public:
	GPairProduct(size_t nMaxDims);

	/// Load from a DOM.
	GPairProduct(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GPairProduct();

	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const;

	/// See the comment for GIncrementalTransform::transform
	virtual void transform(const GVec& in, GVec& out);

	/// Throws an exception (because this transform cannot be reversed).
	virtual void untransform(const GVec& in, GVec& out)
	{ throw Ex("This transformation cannot be reversed"); }

	/// Throws an exception (because this transform cannot be undone).
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut)
	{ throw Ex("This transformation cannot be reversed"); }

protected:
	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GRelation& relation);
};


/// This transforms data by passing it through a multi-layer perceptron
/// with randomely-initialized weights. (This transform automatically
/// converts nominal attributes to categorical as necessary, but it does
/// not normalize. In other words, it assumes that all continuous attributes
/// fall approximately within a 0-1 range.)
class GReservoir : public GIncrementalTransform
{
protected:
	GIncrementalLearner* m_pNN;
	size_t m_outputs;
	double m_deviation;
	size_t m_hiddenLayers;

public:
	GReservoir(double weightDeviation = 2.0, size_t outputs = 64, size_t hiddenLayers = 2);

	/// Load from a DOM.
	GReservoir(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GReservoir();

#ifndef MIN_PREDICT
	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const;
#endif // MIN_PREDICT

	/// See the comment for GIncrementalTransform::transform
	virtual void transform(const GVec& in, GVec& out);

	/// Throws an exception (because this transform cannot be reversed).
	virtual void untransform(const GVec& in, GVec& out)
	{ throw Ex("This transformation cannot be reversed"); }

	/// Throws an exception (because this transform cannot be undone).
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut)
	{ throw Ex("This transformation cannot be reversed"); }

protected:
	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GRelation& relation);
};


/// This class augments data. That is, it transforms the data according to
/// some provided transformation, and attaches the transformed data as new
/// attributes to the existing data.
class GDataAugmenter : public GIncrementalTransform
{
protected:
	GIncrementalTransform* m_pTransform;

public:
	/// Generic constructor. pTransform specifies any data transformation. The transformed
	/// data will be attached to the existing data in order to augment it.
	/// This method takes ownership of pTransform.
	GDataAugmenter(GIncrementalTransform* pTransform);

	/// Load from a DOM
	GDataAugmenter(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GDataAugmenter();

#ifndef MIN_PREDICT
	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const;
#endif // MIN_PREDICT

	/// See the comment for GIncrementalTransform::transform
	virtual void transform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransform
	virtual void untransform(const GVec& in, GVec& out);

	/// Throws an exception
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut);

protected:
	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GRelation& relation);
};


#ifndef MIN_PREDICT
/// Generates subsets of data that contain only the most relevant features for predicting the labels.
/// The train method of this class produces a ranked ordering of the feature attributes by training
/// a single-layer neural network, and deselecting the weakest attribute until all attributes have been
/// deselected. The transform method uses only the highest-ranked attributes.
class GAttributeSelector : public GIncrementalTransform
{
protected:
	size_t m_labelDims;
	size_t m_targetFeatures;
	std::vector<size_t> m_ranks;
	size_t m_seed;

public:
	GAttributeSelector(size_t labelDims, size_t targetFeatures) : GIncrementalTransform(), m_labelDims(labelDims), m_targetFeatures(targetFeatures), m_seed(1234567)
	{
	}

	GAttributeSelector(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GAttributeSelector()
	{
	}

	static void test();

	virtual GDomNode* serialize(GDom* pDoc) const;

	/// See the comment for GIncrementalTransform::transform
	virtual void transform(const GVec& in, GVec& out);

	/// Specifies the number of features to select. (This method must be called
	/// after train.)
	GRelation* setTargetFeatures(size_t n);

	/// Returns a list of attributes in ranked-order. Most important attributes are first. Weakest attributes are last.
	/// (The results are undefined until after train is called.)
	std::vector<size_t>& ranks() { return m_ranks; }

	/// Throws an exception (because this transform cannot be reversed).
	virtual void untransform(const GVec& in, GVec& out)
	{ throw Ex("This transformation cannot be reversed"); }

	/// Throws an exception (because this transform cannot be reversed).
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut)
	{ throw Ex("This transformation cannot be reversed"); }

	/// Sets a random seed to use with this attribute selector
	void setSeed(size_t seed) { m_seed = seed; }

protected:
	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// Throws an exception (because this transform cannot be trained without data)
	virtual GRelation* trainInner(const GRelation& relation);
};
#endif // MIN_PREDICT


/// This is sort-of the opposite of discretize. It converts each nominal attribute to a categorical
/// distribution by representing each value using the corresponding row of the identity matrix. For
/// example, if a certain nominal attribute has 4 possible values, then a value of 3 would be encoded
/// as the vector 0 0 1 0. When predictions are converted back to nominal values, the mode of the
/// categorical distribution is used as the predicted value. (This is similar to Weka's
/// NominalToBinaryFilter.)
class GNominalToCat : public GIncrementalTransform
{
protected:
	size_t m_valueCap;
	GRand* m_pRand;
	std::vector<size_t> m_ranks;
	bool m_preserveUnknowns;

public:
	GNominalToCat(size_t valueCap = 12);

	/// Load from a DOM.
	GNominalToCat(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GNominalToCat();

	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const;

	/// See the comment for GIncrementalTransform::transform
	virtual void transform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransform
	virtual void untransform(const GVec& in, GVec& out);

#ifndef MIN_PREDICT
	/// See the comment for GIncrementalTransform::untransformToDistribution
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut);
#endif // MIN_PREDICT

	/// Makes a mapping from the post-transform attribute indexes to the pre-transform attribute indexes
	void reverseAttrMap(std::vector<size_t>& rmap);

	/// Specify to preserve unknown values. That is, an unknown nominal value will be
	/// converted to a distribution of all unknown real values.
	void preserveUnknowns() { m_preserveUnknowns = true; }

protected:
	GRelation* init();

	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GRelation& relation);
};



/// This transform scales and shifts continuous values
/// to make them fall within a specified range.
class GNormalize : public GIncrementalTransform
{
protected:
	double m_min, m_max;
	GVec m_mins;
	GVec m_ranges;

public:
	/// min and max specify the target range. (The input domain is determined
	/// automatically when train is called.)
	GNormalize(double min = 0.0, double max = 1.0);

	/// Load from a DOM.
	GNormalize(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GNormalize();

	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const;

	/// See the comment for GIncrementalTransform::transform
	virtual void transform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransform
	virtual void untransform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransformToDistribution
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut);

	/// Specify the input min and range values for each attribute
	void setMinsAndRanges(const GRelation& pRel, const GVec& mins, const GVec& ranges);

protected:
	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// Throws an exception (because this transform cannot be trained without data)
	virtual GRelation* trainInner(const GRelation& relation);
};



/// This transform uses buckets to convert continuous data into discrete data.
/// It is common to use GFilter to combine this with your favorite modeler
/// (which only supports discrete values) to create a modeler that can also support
/// continuous values as well.
class GDiscretize : public GIncrementalTransform
{
protected:
	size_t m_bucketsIn, m_bucketsOut;
	GVec m_pMins;
	GVec m_pRanges;

public:
	/// if buckets is less than 0, then it will use the floor of the square root of the number of rows in the data
	GDiscretize(size_t buckets = INVALID_INDEX);

	/// Load from a DOM.
	GDiscretize(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GDiscretize();

	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const;

	/// See the comment for GIncrementalTransform::transform
	virtual void transform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransform
	virtual void untransform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransformToDistribution
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut);

protected:
	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// Throws an exception (because this transform cannot be trained without data)
	virtual GRelation* trainInner(const GRelation& relation);
};



#ifndef MIN_PREDICT
class GImputeMissingVals : public GIncrementalTransform
{
protected:
	GCollaborativeFilter* m_pCF;
	GNominalToCat* m_pNTC;
	const GMatrix* m_pLabels;
	GMatrix* m_pBatch;

public:
	/// General-purpose constructor
	GImputeMissingVals();

	/// Deserializing constructor
	GImputeMissingVals(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GImputeMissingVals();

	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const;

	/// See the comment for GIncrementalTransform::transform
	virtual void transform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransform
	virtual void untransform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransformToDistribution
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut);

	/// Sets the collaborative filter used to impute missing values. Takes
	/// ownership of pCF. If no collaborative filter is set, the default is to use
	/// matrix factorization with some typical parameters.
	void setCollaborativeFilter(GCollaborativeFilter* pCF);

	/// Specify a label matrix that should be appended with the training data.
	/// This object will not delete pLabels. It is expected that pLabels will remain
	/// valid at least until after train is next called.
	void setLabels(const GMatrix* pLabels);

	/// Unlike most other transforms, this one assumes that the matrix passed
	/// to this method is the same that was used to train it. (This assumption
	/// is necessary in order to utilize the additional label information that
	/// may be available at training time, which can be important for imputation.)
	virtual GMatrix* transformBatch(const GMatrix& in);

protected:
	/// See the comment for GIncrementalTransform::train
	virtual GRelation* trainInner(const GMatrix& data);

	/// Throws an exception (because this transform cannot be trained without data)
	virtual GRelation* trainInner(const GRelation& relation);
};
#endif // MIN_PREDICT


/// This transform converts continuous values into logarithmic space.
class GLogify : public GIncrementalTransform
{
public:
	/// General purpose constructor
	GLogify();

	/// Load from a DOM.
	GLogify(GDomNode* pNode, GLearnerLoader& ll);

	virtual ~GLogify();

	/// Marshal this object into a DOM, which can then be converted to a variety of serial formats.
	virtual GDomNode* serialize(GDom* pDoc) const;

	/// See the comment for GIncrementalTransform::transform
	virtual void transform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransform
	virtual void untransform(const GVec& in, GVec& out);

	/// See the comment for GIncrementalTransform::untransformToDistribution
	virtual void untransformToDistribution(const GVec& in, GPrediction* pOut);

protected:
	/// See the comment for GIncrementalTransform::trainInner
	virtual GRelation* trainInner(const GMatrix& data);

	/// See the comment for GIncrementalTransform::trainInner
	virtual GRelation* trainInner(const GRelation& relation);
};

} // namespace GClasses

#endif // __GTRANSFORM_H__

