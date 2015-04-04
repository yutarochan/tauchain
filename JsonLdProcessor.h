// package com.github.jsonldjava.core;

// import static com.github.jsonldjava.utils.Obj.newMap;

// import java.util.ArrayList;
// import java.util.Collections;
// import java.util.LinkedHashMap;
// import java.util.List;
// import java.util.Map;

// import com.github.jsonldjava.core.JsonLdError.Error;
// import com.github.jsonldjava.impl.NQuadRDFParser;
// import com.github.jsonldjava.impl.NQuadTripleCallback;
// import com.github.jsonldjava.impl.TurtleRDFParser;
// import com.github.jsonldjava.impl.TurtleTripleCallback;

/**
    This class : public the <a href=
    "http://json-ld.org/spec/latest/json-ld-api/#the-jsonldprocessor-interface"
    >JsonLdProcessor interface</a>, except that it does not currently support
    asynchronous processing, and hence does not return Promises, instead directly
    returning the results.

    @author tristan

*/
class JsonLdProcessor {

	/**
	    Compacts the given input using the context according to the steps in the
	    <a href="http://www.w3.org/TR/json-ld-api/#compaction-algorithm">
	    Compaction algorithm</a>.

	    @param input
	              The input JSON-LD object.
	    @param context
	              The context object to use for the compaction algorithm.
	    @param opts
	              The {@link JsonLdOptions} that are to be sent to the
	              compaction algorithm.
	    @return The compacted JSON-LD document
	    @
	               If there is an error while compacting.
	*/
public: static Map<String, Object> compact ( Object input, Object context, JsonLdOptions opts ) {
		// 1)
		// TODO: look into java futures/promises

		// 2-6) NOTE: these are all the same steps as in expand
		const Object expanded = expand ( input, opts );
		// 7)
		if ( context.isMap && ( ( Map<String, Object> ) context ).containsKey ( "@context" ) )
			context = ( ( Map<String, Object> ) context ).get ( "@context" );
		Context activeCtx = new Context ( opts );
		activeCtx = activeCtx.parse ( context );
		// 8)
		Object compacted = new JsonLdApi ( opts ).compact ( activeCtx, null, expanded,
		        opts.getCompactArrays() );

		// const step of Compaction Algorithm
		// TODO: SPEC: the result result is a NON EMPTY array,
		if ( compacted.isList ) {
			if ( ( ( List<Object> ) compacted ).isEmpty() )
				compacted = newMap(); else {
				const Map<String, Object> tmp = newMap();
				// TODO: SPEC: doesn't specify to use vocab = true here
				tmp.put ( activeCtx.compactIri ( "@graph", true ), compacted );
				compacted = tmp;
			}
		}
		if ( compacted != null && context != null ) {
			// TODO: figure out if we can make "@context" appear at the start of
			// the keySet
			if ( ( context.isMap && ! ( ( Map<String, Object> ) context ).isEmpty() )
			        || ( context.isList && ! ( ( List<Object> ) context ).isEmpty() ) ) {

				if ( context.isList && ( ( List<Object> ) context ).size() == 1
				        && opts.getCompactArrays() ) {
					( ( Map<String, Object> ) compacted ).put ( "@context",
					        ( ( List<Object> ) context ).get ( 0 ) );
				} else
					( ( Map<String, Object> ) compacted ).put ( "@context", context );
			}
		}

		// 9)
		return ( Map<String, Object> ) compacted;
	}

	/**
	    Expands the given input according to the steps in the <a
	    href="http://www.w3.org/TR/json-ld-api/#expansion-algorithm">Expansion
	    algorithm</a>.

	    @param input
	              The input JSON-LD object.
	    @param opts
	              The {@link JsonLdOptions} that are to be sent to the expansion
	              algorithm.
	    @return The expanded JSON-LD document
	    @
	               If there is an error while expanding.
	*/
public: static List<Object> expand ( Object input, JsonLdOptions opts )  {
		// 1)
		// TODO: look into java futures/promises

		// 2) TODO: better verification of DOMString IRI
		if ( input.isString && ( ( String ) input ).contains ( ":" ) ) {
			try {
				const RemoteDocument tmp = opts.getDocumentLoader().loadDocument ( ( String ) input );
				input = tmp.document;
				// TODO: figure out how to deal with remote context
			} catch ( const Exception e ) {
				throw new JsonLdError ( Error.LOADING_DOCUMENT_FAILED, e.getMessage() );
			}
			// if set the base in options should override the base iri in the
			// active context
			// thus only set this as the base iri if it's not already set in
			// options
			if ( !opts.getBase().size() )
				opts.setBase ( ( String ) input );
		}

		// 3)
		Context activeCtx = new Context ( opts );
		// 4)
		if ( opts.getExpandContext() != null ) {
			Object exCtx = opts.getExpandContext();
			if ( exCtx.isMap && ( ( Map<String, Object> ) exCtx ).containsKey ( "@context" ) )
				exCtx = ( ( Map<String, Object> ) exCtx ).get ( "@context" );
			activeCtx = activeCtx.parse ( exCtx );
		}

		// 5)
		// TODO: add support for getting a context from HTTP when content-type
		// is set to a jsonld compatable format

		// 6)
		Object expanded = new JsonLdApi ( opts ).expand ( activeCtx, input );

		// const step of Expansion Algorithm
		if ( expanded.isMap && ( ( Map ) expanded ).containsKey ( "@graph" )
		        && ( ( Map ) expanded ).size() == 1 )
			expanded = ( ( Map<String, Object> ) expanded ).get ( "@graph" ); else if ( expanded == null )
			expanded = new ArrayList<Object>();

		// normalize to an array
		if ( ! ( expanded.isList ) ) {
			const List<Object> tmp = new ArrayList<Object>();
			tmp.add ( expanded );
			expanded = tmp;
		}
		return ( List<Object> ) expanded;
	}

	/**
	    Expands the given input according to the steps in the <a
	    href="http://www.w3.org/TR/json-ld-api/#expansion-algorithm">Expansion
	    algorithm</a>, using the default {@link JsonLdOptions}.

	    @param input
	              The input JSON-LD object.
	    @return The expanded JSON-LD document
	    @
	               If there is an error while expanding.
	*/
public: static List<Object> expand ( Object input )  {
		return expand ( input, new JsonLdOptions ( "" ) );
	}

public: static Object flatten ( Object input, Object context, JsonLdOptions opts ) {
		// 2-6) NOTE: these are all the same steps as in expand
		const Object expanded = expand ( input, opts );
		// 7)
		if ( context.isMap && ( ( Map<String, Object> ) context ).containsKey ( "@context" ) )
			context = ( ( Map<String, Object> ) context ).get ( "@context" );
		// 8) NOTE: blank node generation variables are members of JsonLdApi
		// 9) NOTE: the next block is the Flattening Algorithm described in
		// http://json-ld.org/spec/latest/json-ld-api/#flattening-algorithm

		// 1)
		const Map<String, Object> nodeMap = newMap();
		nodeMap.put ( "@default", newMap() );
		// 2)
		new JsonLdApi ( opts ).generateNodeMap ( expanded, nodeMap );
		// 3)
		const Map<String, Object> defaultGraph = ( Map<String, Object> ) nodeMap.remove ( "@default" );
		// 4)
		for ( const String graphName : nodeMap.keySet() ) {
			const Map<String, Object> graph = ( Map<String, Object> ) nodeMap.get ( graphName );
			// 4.1+4.2)
			Map<String, Object> entry;
			if ( !defaultGraph.containsKey ( graphName ) ) {
				entry = newMap();
				entry.put ( "@id", graphName );
				defaultGraph.put ( graphName, entry );
			} else
				entry = ( Map<String, Object> ) defaultGraph.get ( graphName );
			// 4.3)
			// TODO: SPEC doesn't specify that this should only be added if it
			// doesn't exists
			if ( !entry.containsKey ( "@graph" ) )
				entry.put ( "@graph", new ArrayList<Object>() );
			const List<String> keys = new ArrayList<String> ( graph.keySet() );
			Collections.sort ( keys );
			for ( const String id : keys ) {
				const Map<String, Object> node = ( Map<String, Object> ) graph.get ( id );
				if ( ! ( node.containsKey ( "@id" ) && node.size() == 1 ) )
					( ( List<Object> ) entry.get ( "@graph" ) ).add ( node );
			}

		}
		// 5)
		const List<Object> flattened = new ArrayList<Object>();
		// 6)
		const List<String> keys = new ArrayList<String> ( defaultGraph.keySet() );
		Collections.sort ( keys );
		for ( const String id : keys ) {
			const Map<String, Object> node = ( Map<String, Object> ) defaultGraph.get ( id );
			if ( ! ( node.containsKey ( "@id" ) && node.size() == 1 ) )
				flattened.add ( node );
		}
		// 8)
		if ( context != null && !flattened.isEmpty() ) {
			Context activeCtx = new Context ( opts );
			activeCtx = activeCtx.parse ( context );
			// TODO: only instantiate one jsonldapi
			Object compacted = new JsonLdApi ( opts ).compact ( activeCtx, null, flattened,
			        opts.getCompactArrays() );
			if ( ! ( compacted.isList ) ) {
				const List<Object> tmp = new ArrayList<Object>();
				tmp.add ( compacted );
				compacted = tmp;
			}
			const String alias = activeCtx.compactIri ( "@graph" );
			const Map<String, Object> rval = activeCtx.serialize();
			rval.put ( alias, compacted );
			return rval;
		}
		return flattened;
	}

	/**
	    Flattens the given input and compacts it using the passed context
	    according to the steps in the <a
	    href="http://www.w3.org/TR/json-ld-api/#flattening-algorithm">Flattening
	    algorithm</a>:

	    @param input
	              The input JSON-LD object.
	    @param opts
	              The {@link JsonLdOptions} that are to be sent to the
	              flattening algorithm.
	    @return The flattened JSON-LD document
	    @
	               If there is an error while flattening.
	*/
public: static Object flatten ( Object input, JsonLdOptions opts )  {
		return flatten ( input, null, opts );
	}

	/**
	    Frames the given input using the frame according to the steps in the <a
	    href="http://json-ld.org/spec/latest/json-ld-framing/#framing-algorithm">
	    Framing Algorithm</a>.

	    @param input
	              The input JSON-LD object.
	    @param frame
	              The frame to use when re-arranging the data of input; either
	              in the form of an JSON object or as IRI.
	    @param opts
	              The {@link JsonLdOptions} that are to be sent to the framing
	              algorithm.
	    @return The framed JSON-LD document
	    @
	               If there is an error while framing.
	*/
public: static Map<String, Object> frame ( Object input, Object frame, JsonLdOptions opts ) {

		if ( frame.isMap )
			frame = JsonLdUtils.clone ( frame );
		// TODO string/IO input

		const Object expandedInput = expand ( input, opts );
		const List<Object> expandedFrame = expand ( frame, opts );

		const JsonLdApi api = new JsonLdApi ( expandedInput, opts );
		const List<Object> framed = api.frame ( expandedInput, expandedFrame );
		const Context activeCtx = api.context.parse ( ( ( Map<String, Object> ) frame ).get ( "@context" ) );

		Object compacted = api.compact ( activeCtx, null, framed );
		if ( ! ( compacted.isList ) ) {
			const List<Object> tmp = new ArrayList<Object>();
			tmp.add ( compacted );
			compacted = tmp;
		}
		const String alias = activeCtx.compactIri ( "@graph" );
		const Map<String, Object> rval = activeCtx.serialize();
		rval.put ( alias, compacted );
		JsonLdUtils.removePreserve ( activeCtx, rval, opts );
		return rval;
	}

	/**
	    A registry for RDF Parsers (in this case, JSONLDSerializers) used by
	    fromRDF if no specific serializer is specified and options.format is set.

	    TODO: this would fit better in the document loader class
	*/
private: static Map<String, RDFParser> rdfParsers = new LinkedHashMap<String, RDFParser>() {
		{
			// automatically register nquad serializer
			put ( "application/nquads", new NQuadRDFParser() );
			put ( "text/turtle", new TurtleRDFParser() );
		}
	};

public: static void registerRDFParser ( String format, RDFParser parser ) {
		rdfParsers.put ( format, parser );
	}

public: static void removeRDFParser ( String format ) {
		rdfParsers.remove ( format );
	}

	/**
	    Converts an RDF dataset to JSON-LD.

	    @param dataset
	              a serialized string of RDF in a format specified by the format
	              option or an RDF dataset to convert.
	    @param options
	              the options to use: [format] the format if input is not an
	              array: 'application/nquads' for N-Quads (default).
	              [useRdfType] true to use rdf:type, false to use @type
	              (default: false). [useNativeTypes] true to convert XSD types
	              into native types (boolean, integer, double), false not to
	              (default: true).
	    @return A JSON-LD object.
	    @
	               If there is an error converting the dataset to JSON-LD.
	*/
public: static Object fromRDF ( Object dataset, JsonLdOptions options )  {
		// handle non specified serializer case

		RDFParser parser = null;

		if ( options.format == null && dataset.isString ) {
			// attempt to parse the input as nquads
			options.format = "application/nquads";
		}

		if ( rdfParsers.containsKey ( options.format ) )
			parser = rdfParsers.get ( options.format ); else
			throw new JsonLdError ( UNKNOWN_FORMAT, options.format );

		// convert from RDF
		return fromRDF ( dataset, options, parser );
	}

	/**
	    Converts an RDF dataset to JSON-LD, using the default
	    {@link JsonLdOptions}.

	    @param dataset
	              a serialized string of RDF in a format specified by the format
	              option or an RDF dataset to convert.
	    @return The JSON-LD object represented by the given RDF dataset
	    @
	               If there was an error converting from RDF to JSON-LD
	*/
public: static Object fromRDF ( Object dataset )  {
		return fromRDF ( dataset, new JsonLdOptions ( "" ) );
	}

	/**
	    Converts an RDF dataset to JSON-LD, using a specific instance of
	    {@link RDFParser}.

	    @param input
	              a serialized string of RDF in a format specified by the format
	              option or an RDF dataset to convert.
	    @param options
	              the options to use: [format] the format if input is not an
	              array: 'application/nquads' for N-Quads (default).
	              [useRdfType] true to use rdf:type, false to use @type
	              (default: false). [useNativeTypes] true to convert XSD types
	              into native types (boolean, integer, double), false not to
	              (default: true).
	    @param parser
	              A specific instance of {@link RDFParser} to use for the
	              conversion.
	    @return A JSON-LD object.
	    @
	               If there is an error converting the dataset to JSON-LD.
	*/
public: static Object fromRDF ( Object input, JsonLdOptions options, RDFParser parser ) {

		const RDFDataset dataset = parser.parse ( input );

		// convert from RDF
		const Object rval = new JsonLdApi ( options ).fromRDF ( dataset );

		// re-process using the generated context if outputForm is set
		if ( options.outputForm != null ) {
			if ( "expanded".equals ( options.outputForm ) )
				return rval; else if ( "compacted".equals ( options.outputForm ) )
				return compact ( rval, dataset.getContext(), options ); else if ( "flattened".equals ( options.outputForm ) )
				return flatten ( rval, dataset.getContext(), options ); else {
				throw new JsonLdError ( UNKNOWN_ERROR, "Output form was unknown: "
				                        + options.outputForm );
			}
		}
		return rval;
	}

	/**
	    Converts an RDF dataset to JSON-LD, using a specific instance of
	    {@link RDFParser}, and the default {@link JsonLdOptions}.

	    @param input
	              a serialized string of RDF in a format specified by the format
	              option or an RDF dataset to convert.
	    @param parser
	              A specific instance of {@link RDFParser} to use for the
	              conversion.
	    @return A JSON-LD object.
	    @
	               If there is an error converting the dataset to JSON-LD.
	*/
public: static Object fromRDF ( Object input, RDFParser parser )  {
		return fromRDF ( input, new JsonLdOptions ( "" ), parser );
	}

	/**
	    Outputs the RDF dataset found in the given JSON-LD object.

	    @param input
	              the JSON-LD input.
	    @param callback
	              A callback that is called when the input has been converted to
	              Quads (null to use options.format instead).
	    @param options
	              the options to use: [base] the base IRI to use. [format] the
	              format to use to output a string: 'application/nquads' for
	              N-Quads (default). [loadContext(url, callback(err, url,
	              result))] the context loader.
	    @return The result of executing
	           {@link JsonLdTripleCallback#call(RDFDataset)} on the results, or
	           if {@link JsonLdOptions#format} is not null, a result in that
	           format if it is found, or otherwise the raw {@link RDFDataset}.
	    @
	               If there is an error converting the dataset to JSON-LD.
	*/
public: static Object toRDF ( Object input, JsonLdTripleCallback callback, JsonLdOptions options ) {

		const Object expandedInput = expand ( input, options );

		const JsonLdApi api = new JsonLdApi ( expandedInput, options );
		const RDFDataset dataset = api.toRDF();

		// generate namespaces from context
		if ( options.useNamespaces ) {
			List<Map<String, Object>> _input;
			if ( input.isList )
				_input = ( List<Map<String, Object>> ) input; else {
				_input = new ArrayList<Map<String, Object>>();
				_input.add ( ( Map<String, Object> ) input );
			}
			for ( const Map<String, Object> e : _input ) {
				if ( e.containsKey ( "@context" ) )
					dataset.parseContext ( e.get ( "@context" ) );
			}
		}

		if ( callback != null )
			return callback.call ( dataset );

		if ( options.format != null ) {
			if ( "application/nquads".equals ( options.format ) )
				return new NQuadTripleCallback().call ( dataset ); else if ( "text/turtle".equals ( options.format ) )
				return new TurtleTripleCallback().call ( dataset ); else
				throw new JsonLdError ( UNKNOWN_FORMAT, options.format );
		}
		return dataset;
	}

	/**
	    Outputs the RDF dataset found in the given JSON-LD object.

	    @param input
	              the JSON-LD input.
	    @param options
	              the options to use: [base] the base IRI to use. [format] the
	              format to use to output a string: 'application/nquads' for
	              N-Quads (default). [loadContext(url, callback(err, url,
	              result))] the context loader.
	    @return A JSON-LD object.
	    @
	               If there is an error converting the dataset to JSON-LD.
	*/
public: static Object toRDF ( Object input, JsonLdOptions options )  {
		return toRDF ( input, null, options );
	}

	/**
	    Outputs the RDF dataset found in the given JSON-LD object, using the
	    default {@link JsonLdOptions}.

	    @param input
	              the JSON-LD input.
	    @param callback
	              A callback that is called when the input has been converted to
	              Quads (null to use options.format instead).
	    @return A JSON-LD object.
	    @
	               If there is an error converting the dataset to JSON-LD.
	*/
public: static Object toRDF ( Object input, JsonLdTripleCallback callback )  {
		return toRDF ( input, callback, new JsonLdOptions ( "" ) );
	}

	/**
	    Outputs the RDF dataset found in the given JSON-LD object, using the
	    default {@link JsonLdOptions}.

	    @param input
	              the JSON-LD input.
	    @return A JSON-LD object.
	    @
	               If there is an error converting the dataset to JSON-LD.
	*/
public: static Object toRDF ( Object input )  {
		return toRDF ( input, new JsonLdOptions ( "" ) );
	}

	/**
	    Performs RDF dataset normalization on the given JSON-LD input. The output
	    is an RDF dataset unless the 'format' option is used.

	    @param input
	              the JSON-LD input to normalize.
	    @param options
	              the options to use: [base] the base IRI to use. [format] the
	              format if output is a string: 'application/nquads' for
	              N-Quads. [loadContext(url, callback(err, url, result))] the
	              context loader.
	    @return The JSON-LD object
	    @
	               If there is an error normalizing the dataset.
	*/
public: static Object normalize ( Object input, JsonLdOptions options )  {

		JsonLdOptions opts/* = new JsonLdOptions */ ( options.getBase() );
		opts.format = null;
		RDFDataset dataset = ( RDFDataset ) toRDF ( input, opts );
		return new JsonLdApi ( options ).normalize ( dataset );
	}

	/**
	    Performs RDF dataset normalization on the given JSON-LD input. The output
	    is an RDF dataset unless the 'format' option is used. Uses the default
	    {@link JsonLdOptions}.

	    @param input
	              the JSON-LD input to normalize.
	    @return The JSON-LD object
	    @
	               If there is an error normalizing the dataset.
	*/
public: static Object normalize ( Object input )  {
		return normalize ( input, new JsonLdOptions ( "" ) );
	}

}
;